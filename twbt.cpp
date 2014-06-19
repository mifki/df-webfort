//
//  twbt.cpp
//  twbt
//
//  Created by Vitaly Pronkin on 14/05/14.
//  Copyright (c) 2014 mifki. All rights reserved.
//

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <stdint.h>
#include <iostream>
#include <map>
#include <vector>
#include <time.h>
#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include "modules/Maps.h"
#include "modules/World.h"
#include "modules/MapCache.h"
#include "modules/Gui.h"
#include "modules/Screen.h"
#include "modules/Buildings.h"
#include "MemAccess.h"
#include "VersionInfo.h"
#include "df/construction.h"
#include "df/block_square_event_frozen_liquidst.h"
#include "df/graphic.h"
#include "df/enabler.h"
#include "df/renderer.h"
#include "df/popup_message.h"
#include "df/building.h"
#include "df/building_type.h"
#include "df/buildings_other_id.h"
#include "df/item.h"
#include "df/item_type.h"
#include "df/items_other_id.h"
#include "df/viewscreen_dwarfmodest.h"
#include "df/viewscreen_setupadventurest.h"
#include "df/viewscreen_dungeonmodest.h"
#include "df/viewscreen_choose_start_sitest.h"
#include "df/viewscreen_new_regionst.h"
#include "df/viewscreen_layer_export_play_mapst.h"
#include "df/viewscreen_layer_world_gen_paramst.h"
#include "df/viewscreen_overallstatusst.h"
#include "df/viewscreen_petst.h"
#include "df/ui_sidebar_mode.h"
#include "tinythread.h"

#include "SDL_events.h"
#include "SDL_keysym.h"
extern "C"
{
extern int SDL_PushEvent( SDL::Event* event );
}

#include <nopoll.h>

#define PLAYTIME 60*15
#define IDLETIME 60*3

typedef float GLfloat;
typedef unsigned int GLuint;

using df::global::world;
using std::string;
using std::vector;
using df::global::enabler;
using df::global::gps;
using df::global::ui;
using df::global::init;

tthread::thread * wsthread;
static void wsthreadmain(void*);

DFHACK_PLUGIN("webfort");

void (*update_tile_old)(df::renderer *r, int x, int y);
void (*render_old)(df::renderer *r);

#ifdef WIN32
__declspec(naked) void render_old_x(df::renderer *r)
{
    __asm {
        push ebp
        mov ebp, esp

        mov ecx, r
        call render_old

        mov esp, ebp
        pop ebp
        ret
    }
}    
#else
#define render_old_x render_old
#endif    

#ifdef WIN32
__declspec(naked) void update_tile_old_x(df::renderer *r, int x, int y)
{
    __asm {
        push ebp
        mov ebp, esp

        push y
        push x

        mov ecx, r
        call update_tile_old

        mov esp, ebp
        pop ebp
        ret
    }   
} 
#else
#define update_tile_old_x update_tile_old
#endif    

struct tileset {
    string small_font_path;
    string large_font_path;
    long small_texpos[16*16], large_texpos[16*16];
};

struct tileref {
    int tilesetidx;
    int tile;
};

static vector< struct tileset > tilesets;

static bool enabled;
static color_ostream *out2;

typedef struct {
    noPollConn *conn;
    bool active;
    unsigned char mod[256*256];
    time_t itime;
    time_t atime;
} Client;

vector< Client* > clients;
int activeidx = -1;

bool is_text_tile(int x, int y, bool &is_map)
{
    const int tile = x * gps->dimy + y;
    df::viewscreen * ws = Gui::getCurViewscreen();

    int32_t w = gps->dimx, h = gps->dimy;

    is_map = false;

    if (strict_virtual_cast<df::viewscreen_dwarfmodest>(ws))
    {
        if (!x || !y || x == w - 1 || y == h - 1)
           return true;

        uint8_t menu_width, area_map_width;
        Gui::getMenuWidth(menu_width, area_map_width);
        int32_t menu_left = w - 1, menu_right = w - 1;

        bool menuforced = (ui->main.mode != df::ui_sidebar_mode::Default || df::global::cursor->x != -30000);

        if ((menuforced || menu_width == 1) && area_map_width == 2) // Menu + area map
        {
            menu_left = w - 56;
            menu_right = w - 25;
        }
        else if (menu_width == 2 && area_map_width == 2) // Area map only
        {
            menu_left = menu_right = w - 25;
        }
        else if (menu_width == 1) // Wide menu
            menu_left = w - 56;
        else if (menuforced || (menu_width == 2 && area_map_width == 3)) // Menu only
            menu_left = w - 32; 

        if (x >= menu_left && x <= menu_right)
        {
            if (menuforced && ui->main.mode == df::ui_sidebar_mode::Burrows && ui->burrows.in_define_mode)
            {
                // Make burrow symbols use graphics font
                if ((y != 12 && y != 13 && !(x == menu_left + 2 && y == 2)) || x == menu_left || x == menu_right) 
                    return true;
            }
            else
                return true;
        }

        is_map = (x > 0 && x < menu_left);

        return false;
    }
    
    if (strict_virtual_cast<df::viewscreen_setupadventurest>(ws))
    {
        df::viewscreen_setupadventurest *s = strict_virtual_cast<df::viewscreen_setupadventurest>(ws);
        if (s->subscreen != df::viewscreen_setupadventurest::Nemesis)
            return true;
        else if (x < 58 || x >= 78 || y == 0 || y >= 21)
            return true;

        return false;
    }
    
    if (strict_virtual_cast<df::viewscreen_dungeonmodest>(ws))
    {
        //df::viewscreen_dungeonmodest *s = strict_virtual_cast<df::viewscreen_dungeonmodest>(ws);
        //TODO

        if (y >= h-2)
            return true;

        return false;
    }

    if (strict_virtual_cast<df::viewscreen_choose_start_sitest>(ws))
    {
        if (y <= 1 || y >= h - 6 || x == 0 || x >= 57)
            return true;

        return false;
    }
        
    if (strict_virtual_cast<df::viewscreen_new_regionst>(ws))
    {
        if (y <= 1 || y >= h - 2 || x <= 37 || x == w - 1)
            return true;

        return false;
    }

    if (strict_virtual_cast<df::viewscreen_layer_export_play_mapst>(ws))
    {
        if (x == w - 1 || x < w - 23)
            return true;

        return false;
    }

    if (strict_virtual_cast<df::viewscreen_overallstatusst>(ws))
    {
        if ((x == 46 || x == 71) && y >= 8)
            return false;

        return true;
    }

    /*if (strict_virtual_cast<df::viewscreen_petst>(ws))
    {
        if (x == 41 && y >= 7)
            return false;

        return true;
    }*/

    return true;
}
unsigned char sc[256*256*5];

#ifdef WIN32
void __stdcall update_tile(int x, int y)
#else
void update_tile(df::renderer *r, int x, int y)
#endif
{
#ifdef WIN32
    df::renderer *r = enabler->renderer;
#endif

    update_tile_old_x(r, x, y);
    if (!enabled)
        return;

    const int tile = x*gps->dimy + y;

    const unsigned char *s = r->screen + tile*4;
    unsigned char *ss = sc + tile*4;
    *(unsigned int*)ss = *(unsigned int*)s;

    bool is_map;
    if (is_text_tile(x, y, is_map))
        ss[2] |= 64;

    for (int i = 0; i < clients.size(); i++)
        clients[i]->mod[tile] = 0;
}

int newwidth, newheight;
volatile bool needsresize, shownextturn;

#ifdef WIN32
void __stdcall render()
#else
void render(df::renderer *r)
#endif
{
#ifdef WIN32
    df::renderer *r = enabler->renderer;
#endif
    if (needsresize)
    {
        enabler->renderer->grid_resize(newwidth,newheight);
        needsresize = false;
    }
    if (shownextturn)
    {
        df::popup_message *popup = new df::popup_message();
        popup->text = "Next player, unpause to continue.";
        popup->color = 1;
        popup->bright = 0;
        world->status.popups.push_back(popup);

        shownextturn = false;        
    }

    //render_old_x(r);
}


void hook()
{
    if (enabled)
        return;

    long **rVtable = (long **)enabler->renderer;

#ifdef WIN32
    HANDLE process = ::GetCurrentProcess();
    DWORD protection = PAGE_READWRITE;
    DWORD oldProtection;
    if ( ::VirtualProtectEx( process, rVtable[0], 4*sizeof(void*), protection, &oldProtection ) )
    {
#endif

    update_tile_old = (void (*)(df::renderer *r, int x, int y))rVtable[0][0];
    rVtable[0][0] = (long)&update_tile;

    render_old = (void(*)(df::renderer *r))rVtable[0][2];
    rVtable[0][2] = (long)&render;

    enabled = true;   

#ifdef WIN32
    VirtualProtectEx( process, rVtable[0], 4*sizeof(void*), oldProtection, &oldProtection );
    }
#endif

}

void unhook()
{
    if (!enabled)
        return;

    df::renderer* renderer = enabler->renderer;
    long **rVtable = (long **)enabler->renderer;

#ifdef WIN32
    HANDLE process = ::GetCurrentProcess();
    DWORD protection = PAGE_READWRITE;
    DWORD oldProtection;
    if ( ::VirtualProtectEx( process, rVtable[0], 4*sizeof(void*), protection, &oldProtection ) )
    {
#endif
    rVtable[0][0] = (long)update_tile_old;
    rVtable[0][2] = (long)render_old;
#ifdef WIN32
    VirtualProtectEx( process, rVtable[0], 4*sizeof(void*), oldProtection, &oldProtection );
    }
#endif

    enabled = false;
    gps->force_full_display_count = true;
}

DFhackCExport command_result plugin_init ( color_ostream &out, vector <PluginCommand> &commands)
{
    out2 = &out;
    
    hook();

    wsthread = new tthread::thread(wsthreadmain, 0);

    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    if (enabled)
        unhook();

    return CR_OK;
}

unsigned char buf[64*1024];

static SDL::Key mapInputCodeToSDL( const uint32_t code )
{
#define MAP(a, b) case a: return b;
    switch (code)
    {
    MAP(96, SDL::K_KP0);
    MAP(97, SDL::K_KP1);
    MAP(98, SDL::K_KP2);
    MAP(99, SDL::K_KP3);
    MAP(100, SDL::K_KP4);
    MAP(101, SDL::K_KP5);
    MAP(102, SDL::K_KP6);
    MAP(103, SDL::K_KP7);
    MAP(104, SDL::K_KP8);
    MAP(105, SDL::K_KP9);
    MAP(144, SDL::K_NUMLOCK);

    MAP(111, SDL::K_KP_DIVIDE);
    MAP(106, SDL::K_KP_MULTIPLY);
    MAP(109, SDL::K_KP_MINUS);
    MAP(107, SDL::K_KP_PLUS);

    MAP(33, SDL::K_PAGEUP);
    MAP(34, SDL::K_PAGEDOWN);
    MAP(35, SDL::K_END);
    MAP(36, SDL::K_HOME);
    MAP(46, SDL::K_DELETE);

    MAP(112, SDL::K_F1);
    MAP(113, SDL::K_F2);
    MAP(114, SDL::K_F3);
    MAP(115, SDL::K_F4);
    MAP(116, SDL::K_F5);
    MAP(117, SDL::K_F6);
    MAP(118, SDL::K_F7);
    MAP(119, SDL::K_F8);
    MAP(120, SDL::K_F9);
    MAP(121, SDL::K_F10);
    MAP(122, SDL::K_F11);
    MAP(123, SDL::K_F12);

    MAP(37, SDL::K_LEFT);
    MAP(39, SDL::K_RIGHT);
    MAP(38, SDL::K_UP);
    MAP(40, SDL::K_DOWN);

    MAP(188, SDL::K_LESS);
    MAP(190, SDL::K_GREATER);

    MAP(13, SDL::K_RETURN);

    //MAP(16, SDL::K_LSHIFT);
    //MAP(17, SDL::K_LCTRL);
    //MAP(18, SDL::K_LALT);

    MAP(27, SDL::K_ESCAPE);
#undef MAP
    }
    if (code <= 177)
        return (SDL::Key)code;
    return SDL::K_UNKNOWN;
}

void simkey(int down, int mod, SDL::Key sym, int unicode)
{
    SDL::Event event;
    memset(&event, 0, sizeof(event));

    event.type = down ? SDL::ET_KEYDOWN : SDL::ET_KEYUP;
    event.key.state = down ? SDL::BTN_PRESSED : SDL::BTN_RELEASED;
    event.key.ksym.mod = (SDL::Mod)mod;
    event.key.ksym.sym = sym;
    event.key.ksym.unicode = unicode;

    SDL_PushEvent(&event);    
}

void setactive(int newidx)
{
    if (newidx >= clients.size())
        newidx = clients.size() > 0 ? 0 : -1;

    activeidx = newidx;
    if (activeidx == -1)
        return;

    Client *newcl = clients[activeidx];
    newcl->active = true;
    newcl->atime = newcl->itime = time(NULL);
    memset(newcl->mod, 0, sizeof(newcl->mod));

    if (!(*df::global::pause_state))
    {
        simkey(1, 0, SDL::K_SPACE, ' ');
        simkey(0, 0, SDL::K_SPACE, ' ');
    }
    //shownextturn = true;

    *out2 << "active " << activeidx << " " << (activeidx == -1 ? "-" : nopoll_conn_host(clients[activeidx]->conn)) << std::endl;    
}

void listener_on_message (noPollCtx * ctx, noPollConn * conn, noPollMsg * msg, noPollPtr user_data)
{
    Client *cl = (Client*) user_data;
    int idx = 0;
    for (int i = 0; i < clients.size(); i++)
    {
        if (clients[i] == cl)
        {
            idx = i;
            break;
        }
    }    

    const unsigned char *mdata = (const unsigned char*) nopoll_msg_get_payload(msg);
    int msz = nopoll_msg_get_payload_size(msg);

    if (mdata[0] == 112 && msz == 3)
    {
        if (cl->active)
        {
            newwidth = mdata[1];
            newheight = mdata[2];
            needsresize = true;
        }
    }
    else if (mdata[0] == 111 && msz == 4)
    {
        if (cl->active)
        {
            cl->itime = time(NULL);

            SDL::Key k = mdata[2] ? (SDL::Key)mdata[2] : mapInputCodeToSDL(mdata[1]);
            if (k != SDL::K_UNKNOWN)
            {
                int jsmods = mdata[3];
                int sdlmods = 0;

                if (jsmods & 1)
                {
                    simkey(1, 0, SDL::K_LALT, 0);
                    sdlmods |= SDL::KMOD_ALT;
                }
                if (jsmods & 2)
                {
                    simkey(1, 0, SDL::K_LSHIFT, 0);
                    sdlmods |= SDL::KMOD_SHIFT;
                }
                if (jsmods & 4)
                {
                    simkey(1, 0, SDL::K_LCTRL, 0);
                    sdlmods |= SDL::KMOD_CTRL;
                }

                simkey(1, sdlmods, k, mdata[2]);
                simkey(0, sdlmods, k, mdata[2]);

                if (jsmods & 1)
                    simkey(0, 0, SDL::K_LALT, 0);
                if (jsmods & 2)
                    simkey(0, 0, SDL::K_LSHIFT, 0);
                if (jsmods & 4)
                    simkey(0, 0, SDL::K_LCTRL, 0);
            }
        }
    }
    /*else if (mdata[0] == 113)
    {
        int x = (((unsigned int)mdata[1]<<8) | mdata[2]);
        int y = (((unsigned int)mdata[3]<<8) | mdata[4]);
        SDL::Event event;
            memset( &event, 0, sizeof(event) );
            event.type = 4;
            event.motion.type = 4;
            event.motion.state = 0;
            event.motion.x = x;
            event.motion.y = y;
        SDL_PushEvent( &event );

        gps->mouse_x = x;
        gps->mouse_y = y;
    }
    else if (mdata[0] == 114)
    {
        SDL::Event event;

        memset( &event, 0, sizeof(event) );
        event.type = SDL::ET_MOUSEBUTTONDOWN;
        event.button.type = 5;//SDL::SDL_MOUSEBUTTONDOWN;
        event.button.which = 0;
        event.button.button = 1;
        event.button.state = SDL::BTN_PRESSED;
        event.button.x = 100;
        event.button.y = 100;
        SDL_PushEvent( &event );

        memset( &event, 0, sizeof(event) );
        event.type = SDL::ET_MOUSEBUTTONUP;
        event.button.type = 5;//SDL::SDL_MOUSEBUTTONUP;
        event.button.which = 0;
        event.button.button = 1;
        event.button.state = SDL::BTN_RELEASED;
        event.button.x = 100;
        event.button.y = 100;
        SDL_PushEvent( &event );

        //nopoll_conn_send_binary (conn, "\0\0\0", 3);
    }*/
    else if (mdata[0] == 115)
    {
        memset(cl->mod, 0, sizeof(cl->mod));
    }
    else
    {
        bool soon = false;
        if (activeidx != -1 && clients.size() > 1)
        {
            time_t now = time(NULL);
            int played = now - clients[activeidx]->atime;
            int idle = now - clients[activeidx]->itime;
            if (played >= PLAYTIME || idle >= IDLETIME)
                setactive(activeidx+1);
            else if (cl->active)
            {
                if (PLAYTIME - played < 60)
                    soon = true;
            }
        }
        int sent = 1;

        unsigned char *b = buf;
        *(b++) = 110;

        int qpos = idx - activeidx;
        if (qpos < 0)
            qpos = -qpos;
        if (soon)
            qpos |= 128;
        *(b++) = qpos;

        *(b++) = gps->dimx;
        *(b++) = gps->dimy;

        unsigned char *emptyb = b;
        unsigned char *mod = cl->mod;

        do
        {
        //int tile = 0;
        for (int y = 0; y < gps->dimy; y++)
        {
            for (int x = 0; x < gps->dimx; x++)
            {
                const int tile = x * gps->dimy + y;
                unsigned char *s = sc + tile*4;
                if (mod[tile])
                    continue;

                *(b++) = x;
                *(b++) = y;
                *(b++) = s[0];
                *(b++) = s[2];

                int bold = (s[3] != 0) * 8;
                int fg   = (s[1] + bold) % 16;

                *(b++) = fg;
                mod[tile] = 1;
            }
        }
    
        if (b == emptyb)
        {
            nopoll_conn_send_binary (conn, "\0", 1);
            //tthread::this_thread::sleep_for(tthread::chrono::milliseconds(1000/60));
        }
        else
        {
            sent = 1;
            nopoll_conn_send_binary (conn, (const char*)buf, (int)(b-buf));
        }
        } while (!sent);
    }

    nopoll_msg_unref(msg);
    return;
}

void listener_on_close (noPollCtx * ctx, noPollConn * conn, noPollPtr user_data)
{
    Client *cl = (Client*) user_data;
    for (int i = 0; i < clients.size(); i++)
    {
        if (clients[i] == cl)
        {
            clients.erase(clients.begin()+i);
            delete cl;

            if (activeidx == i)
                setactive(activeidx);

            break;
        }
    }

    *out2 << "disconnected " << nopoll_conn_host(conn) << " count " << clients.size() << " active " << activeidx << " " << (activeidx == -1 ? "-" : nopoll_conn_host(clients[activeidx]->conn)) << std::endl;    
}

nopoll_bool listener_on_accept (noPollCtx * ctx, noPollConn * conn, noPollPtr user_data)
{
    if (clients.size() >= 100)
        return false;

    Client *cl = new Client;
    cl->conn = conn;
    cl->active = false;

    nopoll_conn_set_on_msg(conn, listener_on_message, cl);
    nopoll_conn_set_on_close(conn, listener_on_close, cl);

    clients.push_back(cl);

    if (activeidx == -1)
        setactive(clients.size() - 1);

    *out2 << "connected " << nopoll_conn_host(conn) << " count " << clients.size() << " active " << activeidx << " " << nopoll_conn_host(clients[activeidx]->conn) << std::endl;    

    return true;
}

void wsthreadmain(void *dummy)
{
    noPollCtx *ctx = nopoll_ctx_new ();
    if (!ctx)
    {
        // error some handling code here
    }

    // create a listener to receive connections on port 1234
    noPollConn *listener = nopoll_listener_new (ctx, "0.0.0.0", "1234");
    if (!nopoll_conn_is_ok(listener)) {
         // some error handling here
    }
 
    // now set a handler that will be called when a message (fragment or not) is received
    //nopoll_ctx_set_on_msg (ctx, listener_on_message, NULL);
    nopoll_ctx_set_on_accept (ctx, listener_on_accept, NULL);
 
    // now call to wait for the loop to notify events 
    nopoll_loop_wait (ctx, 0);    
}