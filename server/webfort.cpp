/*
 * webfort.cpp
 * Web Fortress
 *
 * Created by Vitaly Pronkin on 14/05/14.
 * Copyright (c) 2014 mifki, ISC license.
 */

#include <stdint.h>
#include <iostream>
#include <map>
#include <vector>

#ifdef WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#elif defined(__APPLE__)
#else
    #include <dlfcn.h>
#endif

#include "tinythread.h"

#include "shared.h"

static tthread::thread * wsthread;

typedef float GLfloat;
typedef unsigned int GLuint;

using namespace df::enums;
using df::global::world;
using std::string;
using std::vector;
using df::global::enabler;
using df::global::gps;
using df::global::ui;
using df::global::init;

vector<string> split(const char *str, char c = ' ')
{
    vector<string> result;

    do {
        const char *begin = str;

        while(*str != c && *str) {
            str++;
        }

        result.push_back(string(begin, str));
    } while (0 != *str++);

    return result;
}

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

struct tileref {
    int tilesetidx;
    int tile;
};

struct override {
    bool building;
    bool tiletype;
    int id, type, subtype;
    struct tileref newtile;
};


static bool enabled, texloaded;
static bool has_textfont, has_overrides;
static vector< struct override > *overrides[256];
static struct tileref override_defs[256];
static df::item_flags bad_item_flags;

// Shared in server.h
unsigned char sc[256*256*5];
int newwidth, newheight;
volatile bool needsresize;

// #define IS_SCREEN(_sc) strict_virtual_cast<df::_sc>(ws)
#define IS_SCREEN(_sc) (id == &df::_sc::_identity)

/* Detects if it is safe for a non-privileged user to trigger an ESC keybind.
 * It should not be safe if it would lead to the menu normally accessible by
 * hitting ESC in dwarf mode, as this would give access to keybind changes,
 * fort abandonment etc.
 */
bool is_safe_to_escape()
{
    df::viewscreen * ws = Gui::getCurViewscreen();
    virtual_identity* id = virtual_identity::get(ws);
    if (IS_SCREEN(viewscreen_dwarfmodest) &&
            ui->main.mode == df::ui_sidebar_mode::Default) {
        return false;
    }
    // TODO: adventurer mode
    if (IS_SCREEN(viewscreen_dungeonmodest)) {
    }
    return true;
}

void show_announcement(std::string announcement)
{
    DFHack::Gui::showPopupAnnouncement(announcement);
}

bool is_dwarf_mode()
{
    t_gamemodes gm;
    World::ReadGameMode(gm);
    return gm.g_mode == game_mode::DWARF;
}

void deify(DFHack::color_ostream* raw_out, std::string nick)
{
    if (is_dwarf_mode()) {
        Core::getInstance().runCommand(*raw_out, "deify " + nick);
    }
}

/*
 * The source of this function is taken from mifki's TWBT plugin.
 * If you want to edit it, edit it upstream and then diff it here.
 */
static bool is_text_tile(int x, int y, bool &is_map)
{
    df::viewscreen* ws = Gui::getCurViewscreen();
    virtual_identity* id = virtual_identity::get(ws);
    assert(ws != NULL);

    int32_t w = gps->dimx, h = gps->dimy;

    is_map = false;

    if (!x || !y || x == w - 1 || y == h - 1)
       return has_textfont;

    if (IS_SCREEN(viewscreen_dwarfmodest))
    {
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
                    return has_textfont;
            }
            else
                return has_textfont;
        }

        is_map = (x > 0 && x < menu_left);

        return false;
    }

    if (!has_textfont)
        return false;

    if (IS_SCREEN(viewscreen_dungeonmodest))
    {
        // TODO: Adventure mode

        if (y >= h-2)
            return true;

        return false;
    }

    if (IS_SCREEN(viewscreen_setupadventurest))
    {
        df::viewscreen_setupadventurest *s = static_cast<df::viewscreen_setupadventurest*>(ws);
        if (s->subscreen != df::viewscreen_setupadventurest::Nemesis)
            return true;
        else if (x < 58 || x >= 78 || y == 0 || y >= 21)
            return true;

        return false;
    }

    if (IS_SCREEN(viewscreen_choose_start_sitest))
    {
        if (y <= 1 || y >= h - 6 || x == 0 || x >= 57)
            return true;

        return false;
    }

    if (IS_SCREEN(viewscreen_new_regionst))
    {
        if (y <= 1 || y >= h - 2 || x <= 37 || x == w - 1)
            return true;

        return false;
    }

    if (IS_SCREEN(viewscreen_layer_export_play_mapst))
    {
        if (x == w - 1 || x < w - 23)
            return true;

        return false;
    }

    if (IS_SCREEN(viewscreen_overallstatusst))
    {
        if ((x == 46 || x == 71) && y >= 8)
            return false;

        return true;
    }

    if (IS_SCREEN(viewscreen_movieplayerst))
    {
        df::viewscreen_movieplayerst *s = static_cast<df::viewscreen_movieplayerst*>(ws);
        return !s->is_playing;
    }

    /*if (IS_SCREEN(viewscreen_petst))
    {
        if (x == 41 && y >= 7)
            return false;

        return true;
    }*/

    return true;
}

void write_tile_arrays(df::renderer *r, int x, int y)
{
    const int tile = x * gps->dimy + y;
    const unsigned char *s = r->screen + tile*4;
    unsigned char *ss = sc + tile*4;
    *(unsigned int*)ss = *(unsigned int*)s;

    bool is_map;
    if (is_text_tile(x, y, is_map)) {
        ss[2] |= 64;
    }

    for (auto i = clients.begin(); i != clients.end(); i++) {
        i->second->mod[tile] = 0;
    }
}

#ifdef WIN32
void __stdcall update_tile(int x, int y)
#else
void update_tile(df::renderer *r, int x, int y)
#endif
{
#ifdef WIN32
    df::renderer *r = enabler->renderer;
#endif

    write_tile_arrays(r, x, y);
    if (!enabled || !texloaded) {
        update_tile_old_x(r, x, y);
        return;
    }


}

#ifdef WIN32
void __stdcall render()
#else
void render(df::renderer *r)
#endif
{
#ifdef WIN32
    df::renderer *r = enabler->renderer;
#endif

    render_old_x(r);
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

    enabled = false;

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

    gps->force_full_display_count = true;
}

bool get_font_paths()
{
    string small_font_path, gsmall_font_path;
    string large_font_path, glarge_font_path;

    std::ifstream fseed("data/init/init.txt");
    if(fseed.is_open()) {
        string str;

        while(std::getline(fseed,str)) {
            size_t b = str.find("[");
            size_t e = str.rfind("]");

            if (b == string::npos || e == string::npos || str.find_first_not_of(" ") < b)
                continue;

            str = str.substr(b+1, e-1);
            vector<string> tokens = split(str.c_str(), ':');

            if (tokens.size() != 2)
                continue;

            if(tokens[0] == "FONT") {
                small_font_path = "data/art/" + tokens[1];
                continue;
            }

            if(tokens[0] == "FULLFONT") {
                large_font_path = "data/art/" + tokens[1];
                continue;
            }

            if(tokens[0] == "GRAPHICS_FONT") {
                gsmall_font_path = "data/art/" + tokens[1];
                continue;
            }

            if(tokens[0] == "GRAPHICS_FULLFONT") {
                glarge_font_path = "data/art/" + tokens[1];
                continue;
            }
        }
    }

    fseed.close();

    return true;
}

bool load_overrides()
{
    bool found = false;

    std::ifstream fseed("data/init/overrides.txt");
    if(fseed.is_open()) {
        string str;

        while(std::getline(fseed,str)) {
            size_t b = str.find("[");
            size_t e = str.rfind("]");

            if (b == string::npos || e == string::npos || str.find_first_not_of(" ") < b)
                continue;

            str = str.substr(b+1, e-1);
            vector<string> tokens = split(str.c_str(), ':');

            if (tokens[0] == "TILESET") {
                continue;
            }

            if (tokens[0] == "OVERRIDE") {
                if (tokens.size() == 6) {
                    int tile = atoi(tokens[1].c_str());
                    if (tokens[2] != "T")
                        continue;

                    struct override o;
                    o.tiletype = true;

                    tiletype::tiletype type;
                    if (find_enum_item(&type, tokens[3]))
                        o.type = type;
                    else
                        continue;

                    o.newtile.tilesetidx = atoi(tokens[4].c_str());
                    o.newtile.tile = atoi(tokens[5].c_str());

                    if (!overrides[tile])
                        overrides[tile] = new vector< struct override >;
                    overrides[tile]->push_back(o);
                } else if (tokens.size() == 8) {
                    int tile = atoi(tokens[1].c_str());

                    struct override o;
                    o.tiletype = false;
                    o.building = (tokens[2] == "B");
                    if (o.building) {
                        buildings_other_id::buildings_other_id id;
                        if (find_enum_item(&id, tokens[3]))
                            o.id = id;
                        else
                            o.id = -1;

                        building_type::building_type type;
                        if (find_enum_item(&type, tokens[4]))
                            o.type = type;
                        else
                            o.type = -1;
                    } else {
                        items_other_id::items_other_id id;
                        if (find_enum_item(&id, tokens[3]))
                            o.id = id;
                        else
                            o.id = -1;

                        item_type::item_type type;
                        if (find_enum_item(&type, tokens[4]))
                            o.type = type;
                        else
                            o.type = -1;
                    }

                    if (tokens[5].length() > 0)
                        o.subtype = atoi(tokens[5].c_str());
                    else
                        o.subtype = -1;

                    o.newtile.tilesetidx = atoi(tokens[6].c_str());
                    o.newtile.tile = atoi(tokens[7].c_str());

                    if (!overrides[tile])
                        overrides[tile] = new vector< struct override >;
                    overrides[tile]->push_back(o);

                } else if (tokens.size() == 4) {
                    int tile = atoi(tokens[1].c_str());
                    override_defs[tile].tilesetidx = atoi(tokens[2].c_str());
                    override_defs[tile].tile = atoi(tokens[3].c_str());
                }

                found = true;
                continue;
            }
        }
    }

    fseed.close();
    return found;
}

/*
 * probably dead code. upstream has this ifdef'd to 34.11.0
 * https://github.com/mifki/df-twbt/blob/master/tradefix.hpp
 */
#if defined(__APPLE__) && defined(TRADERESIZE)
//0x0079cb2a+4 0x14 - item name length
//0x0079cb18+3 0x14 - item name length

//0x0079e04e+3 0x1a - price, affects both sides
//0x0079cbd1+2 0x1f - weight, affects both sides
//0x0079ccbc+2 0x21 - [T], affects both sides

//0x0079ef96+2 0x29 - "Value:"
//0x0079d84d+2 0x39 - "Max weight"
//0x0079d779+2 0x2a - "offer marked"
//0x0079d07c+2 0x2a - "view good"

//0x0079c314+1 0x3b - our side name (center)
//0x0079cd6b+7 0x28 - our item name (+2)

//0x002e04cf+2 0x27 - border left
//0x002e0540+2 XXXX - border right

struct traderesize_hook : public df::viewscreen_tradegoodsst
{
    typedef df::viewscreen_tradegoodsst interpose_base;

    DEFINE_VMETHOD_INTERPOSE(void, render, ())
    {
        static bool checked = false, ok = false;

        if (!checked)
        {
            checked = true;

            //check only some of the addresses
            ok =
                *(unsigned char*)(0x002e04cf+2) == 0x27 &&
                *(unsigned char*)(0x0079ef96+2) == 0x29 &&
                *(unsigned char*)(0x0079cbd1+2) == 0x1f &&
                *(unsigned char*)(0x0079cb2a+4) == 0x14;

            if (ok)
            {
                //fixing drawing of the right border
                unsigned char t1[] = { 0x6b, 0xd1, 0x00 }; //imul edx, ecx, XXX
                Core::getInstance().p->patchMemory((void*)(0x002e0540), t1, sizeof(t1));

                unsigned char t2[] = { 0x01, 0xf2, 0x90 }; //add edx, esi; nop
                Core::getInstance().p->patchMemory((void*)(0x002e0545), t2, sizeof(t2));
            }
        }

        if (ok)
        {
            static int lastw = -1;
            if (gps->dimx != lastw)
            {
                lastw = gps->dimx;

                unsigned char x1 = lastw/2-1, x;

                //border
                x = x1;
                Core::getInstance().p->patchMemory((void*)(0x002e04cf+2), &x, 1);
                x = x1 + 1;
                Core::getInstance().p->patchMemory((void*)(0x002e0540+2), &x, 1);

                x = x1 + 1 + 2;
                Core::getInstance().p->patchMemory((void*)(0x0079d07c+2), &x, 1); //view good
                Core::getInstance().p->patchMemory((void*)(0x0079d779+2), &x, 1); //offer marked

                x = x1 + 1 + 1;
                Core::getInstance().p->patchMemory((void*)(0x0079ef96+2), &x, 1); //value

                x = x1 + 1 + 1 + 16;
                Core::getInstance().p->patchMemory((void*)(0x0079d84d+2), &x, 1); //max weight

                x = x1 + 1 + 2 - 2;
                Core::getInstance().p->patchMemory((void*)(0x0079cd6b+7), &x, 1); //item name

                x = x1 - 2 - 3;
                Core::getInstance().p->patchMemory((void*)(0x0079ccbc+2), &x, 1); //[T]

                x = x1 - 2 - 3;
                Core::getInstance().p->patchMemory((void*)(0x0079cbd1+2), &x, 1); //item weight

                x = x1 - 2 - 3 - 5;
                Core::getInstance().p->patchMemory((void*)(0x0079e04e + 3), &x, 1); //item price

                x = x1 - 2 - 3 - 5 - 5;
                Core::getInstance().p->patchMemory((void*)(0x0079cb2a+4), &x, 1); //item name len
                Core::getInstance().p->patchMemory((void*)(0x0079cb18+3), &x, 1); //item name len

                x = (x1 + 2 + lastw) / 2;
                Core::getInstance().p->patchMemory((void*)(0x0079c314+1), &x, 1); //our side name
            }
        }

        INTERPOSE_NEXT(render)();
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(traderesize_hook, render);
#endif // TRADERESIZE

DFhackCExport command_result plugin_init ( color_ostream &out, vector <PluginCommand> &commands)
{
    auto dflags = init->display.flag;
    if (dflags.is_set(init_display_flags::TEXT)) {
        out.color(COLOR_RED);
        out << "Webfort: PRINT_MODE must not be TEXT" << std::endl;
        out << "Webfort: Aborting." << std::endl;
        out.color(COLOR_RESET);
        return CR_OK;
    }

    bad_item_flags.whole = 0;
    bad_item_flags.bits.in_building = true;
    bad_item_flags.bits.garbage_collect = true;
    bad_item_flags.bits.removed = true;
    bad_item_flags.bits.dead_dwarf = true;
    bad_item_flags.bits.murder = true;
    bad_item_flags.bits.construction = true;
    bad_item_flags.bits.in_inventory = true;
    bad_item_flags.bits.in_chest = true;

    //Main tileset
    memset(override_defs, 0, sizeof(struct tileref)*256);

    has_textfont = get_font_paths();
    has_overrides |= load_overrides();
    if (has_textfont || has_overrides) {
        hook();
    }
    if (!has_textfont) {
        out.color(COLOR_YELLOW);
        out << "Webfort: FONT and GRAPHICS_FONT are the same" << std::endl;
        out.color(COLOR_RESET);
    }

#ifdef __APPLE__
    INTERPOSE_HOOK(traderesize_hook, render).apply(true);
#endif

    wsthread = new tthread::thread(wsthreadmain, &out);

    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    if (enabled)
        unhook();

#ifdef __APPLE__
    INTERPOSE_HOOK(traderesize_hook, render).apply(false);
#endif

    return CR_OK;
}

/* vim: set et sw=4 : */
