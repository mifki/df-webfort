#ifndef __WF_SERVER_H__
#define __WF_SERVER_H__

/*
 * server.h
 * Part of Web Fortress
 *
 * Copyright (c) 2014 mifki, ISC license.
 */

#include <ctime>
#include <vector>

typedef struct {
    void* hook;
    unsigned char mod[256*256];
    time_t itime;
    time_t atime;
} Client;

extern std::vector<Client*> clients;

extern unsigned char sc[256*256*5];
extern int newwidth, newheight;
extern volatile bool needsresize;

void wsthreadmain(void*);
#endif
