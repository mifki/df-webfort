#ifndef __WF_SERVER_H__
#define __WF_SERVER_H__

/*
 * server.h
 * Part of Web Fortress
 *
 * Copyright (c) 2014 mifki, ISC license.
 */

#include <ctime>
#include <map>
#include <string>
#include <websocketpp/server.hpp>

typedef struct {
    std::string name;
    unsigned char mod[256*256];
    time_t itime;
    time_t atime;
} Client;

typedef std::map<websocketpp::connection_hdl, Client, std::owner_less<websocketpp::connection_hdl>> conn_map;
extern conn_map clients;

extern unsigned char sc[256*256*5];
extern int newwidth, newheight;
extern volatile bool needsresize;

void wsthreadmain(void*);
#endif
