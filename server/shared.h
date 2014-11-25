#ifndef __WF_SHARED_H_
#define __WF_SHARED_H__

/*
 * shared.h
 * Part of Web Fortress
 *
 * Copyright (c) 2014 mifki, ISC license.
 */

#include <ctime>
#include <map>
#include <string>
#include <websocketpp/server.hpp>

typedef struct {
    std::string addr;
    std::string nick;
    unsigned char mod[256*256];
    time_t atime;
} Client;

// FIXME: use unique_ptr or the boost equivalent
typedef std::map<websocketpp::connection_hdl, Client*, std::owner_less<websocketpp::connection_hdl>> conn_map;
extern conn_map clients;

extern unsigned char sc[256*256*5];
extern int newwidth, newheight;
extern volatile bool needsresize;

void wsthreadmain(void*);

bool is_safe_to_escape();
void show_announcement(std::string announcement);

/*
 * DFHack Includes
 * The includes that were commented out were done simply by a process of
 * "does this break the build? if not, comment it out." They might be
 * transitively included, but commenting them out speeds things up anyways.
 */

#include "PluginManager.h"
#include "VersionInfo.h"
#include "VTableInterpose.h"
#include "modules/MapCache.h"
#include "modules/Gui.h"
#include "modules/World.h"
#include "df/graphic.h"
#include "df/enabler.h"
#include "df/renderer.h"
#include "df/building.h"
#include "df/buildings_other_id.h"
#include "df/unit.h"
#include "df/items_other_id.h"
#include "df/viewscreen_dwarfmodest.h"
#include "df/viewscreen_setupadventurest.h"
#include "df/viewscreen_dungeonmodest.h"
#include "df/viewscreen_choose_start_sitest.h"
#include "df/viewscreen_new_regionst.h"
#include "df/viewscreen_layer_export_play_mapst.h"
#include "df/viewscreen_overallstatusst.h"
#include "df/viewscreen_movieplayerst.h"

using namespace DFHack;

// #include "Core.h"
// #include "Console.h"
// #include "Export.h"
// #include "modules/Maps.h"
// #include "modules/World.h"
// #include "modules/Screen.h"
// #include "modules/Buildings.h"
// #include "MemAccess.h"
// #include "df/construction.h"
// #include "df/block_square_event_frozen_liquidst.h"
// #include "df/building_type.h"
// #include "df/item.h"
// #include "df/item_type.h"
// #include "df/tiletype.h"
// #include "df/viewscreen_layer_world_gen_paramst.h"
// #include "df/viewscreen_tradegoodsst.h"
// #include "df/viewscreen_petst.h"

void deify(DFHack::color_ostream* raw_out, std::string nick);
void quicksave(DFHack::color_ostream* out);
#endif
