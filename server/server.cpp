/*
 * server.cpp
 * Part of Web Fortress
 *
 * Copyright (c) 2014 mifki, ISC license.
 */

#include "nopoll.h"
#include "server.h"

#include "ColorText.h"
#include "modules/Gui.h"
#include "df/graphic.h"

#include "SDL_events.h"
#include "SDL_keysym.h"
extern "C"
{
extern int SDL_PushEvent( SDL::Event* event );
}

using df::global::gps;

#define PLAYTIME 60*10
#define IDLETIME 60*3
#define LISTENER "0.0.0.0"
#define PORT "1234"

std::vector<Client*> clients;

static int activeidx = -1;
static DFHack::color_ostream *out2;
static unsigned char buf[64*1024];

static SDL::Key mapInputCodeToSDL( const uint32_t code )
{
#define MAP(a, b) case a: return b;
    switch (code)
    {
    // {{{ keysyms
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
    // }}}
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
    if (newidx >= clients.size()) {
        newidx = -1;
    }

    activeidx = newidx;

    if (activeidx != -1) {
        Client *newcl = clients[activeidx];

        newcl->atime = newcl->itime = time(NULL);
        memset(newcl->mod, 0, sizeof(newcl->mod));
    }

    if (!(*df::global::pause_state)) {
        simkey(1, 0, SDL::K_SPACE, ' ');
        simkey(0, 0, SDL::K_SPACE, ' ');
    }
    const char* host = nopoll_conn_host((noPollConn*)clients[activeidx]->hook);
    *out2 << "active " << activeidx << " " << (activeidx == -1 ? "-" : host) << std::endl;
}

void tock(noPollConn* conn, int idx)
{
    // Tock
    Client* cl = clients[idx];
    int32_t time_left = -1;
    if (activeidx != -1 && clients.size() > 1)
    {
        time_t now = time(NULL);
        int played = now - clients[activeidx]->atime;
        int idle = now - clients[activeidx]->itime;
        if (played >= PLAYTIME || idle >= IDLETIME) {
            setactive(-1);
            time_left = -1;
        } else {
            time_left = PLAYTIME - played;
        }
    }
    int sent = 1;

    unsigned char *b = buf;
    // [0] msgtype
    *(b++) = 110;

    uint8_t client_count = clients.size();
    if (idx == activeidx) {
        client_count |= 128;
    }
    // [1] # of connected clients. 128 bit set if client is active player.
    *(b++) = client_count;

    // [2-5] time left, in seconds. -1 if no player.
    memcpy(b, &time_left, sizeof(time_left));
    b += sizeof(time_left);

    // [6-7] game dimensions
    *(b++) = gps->dimx;
    *(b++) = gps->dimy;

    unsigned char *emptyb = b;
    unsigned char *mod = cl->mod;

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

    nopoll_conn_send_binary (conn, (const char*)buf, (int)(b-buf));
}

int getClientIndex(Client* cl)
{
    int idx = -1;
    for (int i = 0; i < clients.size(); i++)
    {
        if (clients[i] == cl)
        {
            idx = i;
            break;
        }
    }
    return idx;
}

void listener_on_message (noPollCtx * ctx, noPollConn * conn, noPollMsg * msg, noPollPtr user_data)
{
    Client *cl = (Client*) nopoll_conn_get_hook(conn);
    int idx = getClientIndex(cl);

    const unsigned char *mdata = (const unsigned char*) nopoll_msg_get_payload(msg);
    int msz = nopoll_msg_get_payload_size(msg);

    if (mdata[0] == 112 && msz == 3) // ResizeEvent
    {
        if (idx == activeidx)
        {
            newwidth = mdata[1];
            newheight = mdata[2];
            needsresize = true;
        }
    }
    else if (mdata[0] == 111 && msz == 4) // KeyEvent
    {
        if (idx == activeidx)
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
    else if (mdata[0] == 115) // ModifierEvent
    {
        memset(cl->mod, 0, sizeof(cl->mod));
    }
    else if (mdata[0] == 116) // requestTurn
    {
        if (activeidx == idx) {
            setactive(-1);
        } else if (activeidx == -1) {
            setactive(idx);
        }
    }
    else
    {
        tock(conn, idx);
    }

    nopoll_msg_unref(msg);
    return;
}

void listener_on_close (noPollCtx * ctx, noPollConn * conn, noPollPtr user_data)
{
    Client *cl = (Client*) nopoll_conn_get_hook(conn);
    for (int i = 0; i < clients.size(); i++)
    {
        if (clients[i] == cl)
        {
            clients.erase(clients.begin()+i);
            delete cl;

            if (activeidx == i) {
                setactive(-1);
            }

            break;
        }
    }

    const char* host = nopoll_conn_host((noPollConn*)clients[activeidx]->hook);

    *out2 << "disconnected " << nopoll_conn_host(conn) << " count " << clients.size() << " active " << activeidx << " " << (activeidx == -1 ? "-" : host) << std::endl;
}

nopoll_bool listener_on_accept (noPollCtx * ctx, noPollConn * conn, noPollPtr user_data)
{
    if (clients.size() >= 100)
        return false;

    Client *cl = new Client;
    cl->hook = (void*) conn;
    nopoll_conn_set_hook(conn, cl);

    nopoll_conn_set_on_msg(conn, listener_on_message, NULL);
    nopoll_conn_set_on_close(conn, listener_on_close, NULL);

    clients.push_back(cl);

    *out2 << "connected " << nopoll_conn_host(conn) << " count " << clients.size() << std::endl;

    return true;
}

void wsthreadmain(void *out)
{
	out2 = (DFHack::color_ostream*) out;
    noPollConn* listener;
    noPollCtx *ctx = nopoll_ctx_new ();
    if (!ctx)
    {
        *out2 << "Error: Web Fortress failed to load new context.\n";
        return;
    }

    // create a listener to receive connections
    listener = nopoll_listener_new(ctx, LISTENER, PORT);
    if (!nopoll_conn_is_ok(listener)) {
        *out2 << "Error: Web Fortress failed to open socket " << LISTENER << ":" << PORT <<
            ". Is another instance running?\n";
        return;
    }

    // now set a handler that will be called when a message (fragment or not) is received
    nopoll_ctx_set_on_accept (ctx, listener_on_accept, NULL);

    *out2 << "Web Fortress is ready on " << LISTENER << ":" << PORT << ".\n";
    out2->flush();
    // now call to wait for the loop to notify events
    nopoll_loop_wait (ctx, 0);
}
