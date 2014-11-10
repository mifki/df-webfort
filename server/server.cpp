/*
 * server.cpp
 * Part of Web Fortress
 *
 * Copyright (c) 2014 mifki, ISC license.
 */

#include "shared.h"
#include <cassert>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

namespace ws  = websocketpp;
namespace lib = websocketpp::lib;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

typedef ws::server<ws::config::asio> server;

typedef server::message_ptr message_ptr;

using df::global::gps;

#define IDLETIME 60*3
// TURNTIME needs to fit at least a uint32
int64_t TURNTIME = 600; // 10 minutes
uint32_t MAX_CLIENTS = 32;
uint16_t PORT = 1234;

conn_map clients;

static ws::connection_hdl null_conn = std::weak_ptr<void>();
static Client* null_client;

static ws::connection_hdl active_conn = null_conn;
typedef ws::connection_hdl conn;

static std::owner_less<std::weak_ptr<void>> conn_lt;
static bool conn_eq(conn p, conn q)
{
    return (!conn_lt(p, q) && !conn_lt(q, p));
}

static unsigned char buf[64*1024];

/* FIXME: input handling is long-winded enough to get its own file. */
#include "SDL_events.h"
#include "SDL_keysym.h"
extern "C"
{
extern int SDL_PushEvent( SDL::Event* event );
}
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

static std::ostream* out2;
class logbuf : public std::stringbuf {
public:
    logbuf(DFHack::color_ostream* i_out) : std::stringbuf()
    {
        dfout = i_out;
    }
    int sync()
    {
        // TODO: tidy up logs for human consumption
        std::string o = "[WEBFORT] " + this->str();
        size_t i = -1;
        // remove empty lines.
        while ((i = o.find("\n\n")) != std::string::npos) {
            o.replace(i, 2, "\n");
        }

        *dfout << o;
        std::cout << o;

        dfout->flush();
        std::cout.flush();
        str("");
        return 0;
    }
private:
    DFHack::color_ostream* dfout;
};

class appbuf : public std::stringbuf {
public:
    appbuf(server* i_srv) : std::stringbuf()
    {
        srv = i_srv;
    }
    int sync()
    {
        srv->get_alog().write(ws::log::alevel::app, this->str());
        str("");
        return 0;
    }
private:
    server* srv;
};

Client* get_client(conn hdl)
{
    auto it = clients.find(hdl);
    if (it == clients.end()) {
        return null_client;
    }
    return it->second;
}

void set_active(conn newc)
{
    if (conn_eq(active_conn, newc)) { return; }
    Client* newcl = get_client(newc); // fail early
    active_conn = newc;

    if (!conn_eq(active_conn, null_conn)) {
        newcl->atime = time(NULL);
        memset(newcl->mod, 0, sizeof(newcl->mod));
    }

    if (!(*df::global::pause_state)) {
        simkey(1, 0, SDL::K_SPACE, ' ');
        simkey(0, 0, SDL::K_SPACE, ' ');
    }

    *out2 << newcl->nick << " is now active." << std::endl;
}

bool validate_open(server* s, conn hdl)
{
    auto WF_VERSION = "WebFortress-v2.0"; // FIXME: compiler flag.
    auto raw_conn = s->get_con_from_hdl(hdl);

    std::vector<std::string> protos = raw_conn->get_requested_subprotocols();
    if (std::find(protos.begin(), protos.end(), WF_VERSION) == protos.end()) {
        raw_conn->set_body("Invalid subprotocol.");
        return false;
    }
    raw_conn->select_subprotocol(WF_VERSION);

    if (clients.size() >= MAX_CLIENTS) {
        raw_conn->set_body("Server full.");
        return false;
    }

    if (raw_conn->get_resource() == "/__NOBODY") {
        raw_conn->set_body("Invalid Nickname.");
        return false;
    }

    return true;
}

void on_open(server* s, conn hdl)
{
    auto cl = new Client;

    auto raw_conn = s->get_con_from_hdl(hdl);
    cl->addr = raw_conn->get_remote_endpoint();
    cl->nick = raw_conn->get_resource().substr(1); // remove leading '/'

    cl->atime = time(NULL);
    memset(cl->mod, 0, sizeof(cl->mod));

    clients[hdl] = cl;
}

void on_close(server* s, conn c)
{
    Client* cl = get_client(c);
    if (conn_eq(c, active_conn)) {
        set_active(null_conn);
    }
    clients.erase(c);
    delete cl;
}

void tock(server* s, conn hdl)
{
    Client* cl = get_client(hdl);
    Client* active_cl = get_client(active_conn);
    int32_t time_left = -1;

    if (TURNTIME != 0 && !conn_eq(active_conn, null_conn) && clients.size() > 1)
    {
        time_t now = time(NULL);
        int played = now - active_cl->atime;
        if (played < TURNTIME) {
            time_left = TURNTIME - played;
        } else {
            *out2 << active_cl->nick << " has run out of time." << std::endl;
            set_active(null_conn);
        }
    }

    unsigned char *b = buf;
    // [0] msgtype
    *(b++) = 110;

    uint8_t client_count = clients.size();
    if (conn_eq(hdl, active_conn)) {
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

    // [8] Length of current active player's nick, including '\0'.
    uint8_t nick_len = active_cl->nick.length() + 1;
    *(b++) = nick_len;

    unsigned char *mod = cl->mod;

    // [9-M] null-terminated string: active player's nick
    memcpy(b, active_cl->nick.c_str(), nick_len);
    b += nick_len;

    // [M-N] Changed tiles. 5 bytes per tile
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
    s->send(hdl, (const void*) buf, (size_t)(b-buf), ws::frame::opcode::binary);
}


void on_message(server* s, conn hdl, message_ptr msg)
{
    auto str = msg->get_payload();
    const unsigned char *mdata = (const unsigned char*) str.c_str();
    int msz = str.size();

    if (mdata[0] == 112 && msz == 3) // ResizeEvent
    {
        if (conn_eq(hdl, active_conn))
        {
            newwidth = mdata[1];
            newheight = mdata[2];
            needsresize = true;
        }
    }
    else if (mdata[0] == 111 && msz == 4) // KeyEvent
    {
        if (conn_eq(hdl, active_conn))
        {
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
    else if (mdata[0] == 115) // refreshScreen
    {
        Client* cl = get_client(hdl);
        memset(cl->mod, 0, sizeof(cl->mod));
    }
    else if (mdata[0] == 116) // requestTurn
    {
        assert(conn_eq(active_conn, active_conn));
        assert(!conn_eq(hdl, null_conn));
        if (conn_eq(hdl, active_conn)) {
            set_active(null_conn);
        } else if (conn_eq(active_conn, null_conn)) {
            set_active(hdl);
        }
    }
    else
    {
        tock(s, hdl);
    }

    return;
}

void on_init(conn hdl, boost::asio::ip::tcp::socket & s)
{
    s.set_option(boost::asio::ip::tcp::no_delay(true));
}

void wsthreadmain(void *out)
{
    null_client = new Client;
    null_client->nick = "__NOBODY";

    logbuf lb((DFHack::color_ostream*) out);
    std::ostream logstream(&lb);

    server srv;

    char* tmp;

    try {
        // FIXME: bounds checking.
        if ((tmp = getenv("WF_PORT"))) {
            PORT = (uint16_t)std::stol(tmp);
        }
        if ((tmp = getenv("WF_TURNTIME"))) {
            TURNTIME = (int64_t)std::stol(tmp);
        }
        if ((tmp = getenv("WF_MAX_CLIENTS"))) {
            MAX_CLIENTS = (uint32_t)std::stol(tmp);
        }

        srv.clear_access_channels(ws::log::alevel::all);
        srv.set_access_channels(
                ws::log::alevel::connect    |
                ws::log::alevel::disconnect |
                ws::log::alevel::app
        );
        srv.set_error_channels(
                ws::log::elevel::info   |
                ws::log::elevel::warn   |
                ws::log::elevel::rerror |
                ws::log::elevel::fatal
        );
        srv.init_asio();

        srv.get_alog().set_ostream(&logstream);
        appbuf abuf(&srv);
        std::ostream astream(&abuf);
        out2 = &astream;

        srv.set_socket_init_handler(&on_init);
        srv.set_validate_handler(bind(&validate_open, &srv, ::_1));
        srv.set_open_handler(bind(&on_open, &srv, ::_1));
        srv.set_message_handler(bind(&on_message, &srv, ::_1, ::_2));
        srv.set_close_handler(bind(&on_close, &srv, ::_1));

        lib::error_code ec;
        srv.listen(PORT, ec);
        if (ec) {
            *out2 << "Unable to start Webfort on port " << PORT
                  << ", is it being used somehere else?" << std::endl;
            return;
        }

        srv.start_accept();
        // Start the ASIO io_service run loop
        *out2 << "Web Fortress started on port " << PORT << std::endl;
        srv.run();
    } catch (const std::exception & e) {
        *out2 << "Webfort failed to start: " << e.what() << std::endl;
    } catch (lib::error_code e) {
        *out2 << "Webfort failed to start: " << e.message() << std::endl;
    } catch (...) {
        *out2 << "Webfort failed to start: other exception" << std::endl;
    }
    return;
}
