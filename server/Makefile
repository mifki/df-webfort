DFHACKVER ?= 0.34.11
DFHACKREL ?= r5

DF ?= df_linux
DH ?= dfhack

WSPP_I ?= -Iwebsocketpp
WSPP_L ?= -lboost_system -lz -lpthread

SRC = webfort.cpp server.cpp
DEP =
OUT = dist/dfhack-$(DFHACKREL)/webfort.plug.so

SDL_I = $(shell sdl-config --cflags)

INC = -I"$(DH)/library/include" -I"$(DH)/library/proto" -I"$(DH)/depends/protobuf" -I"$(DH)/depends/tthread" $(WSPP_I) $(SDL_I)
LIB = -L"$(DH)/build/library" -ldfhack -L"$(DH)/build/depends/tthread" -ldfhack-tinythread -L"/usr/lib/i386-linux-gnu" $(WSPP_L)

CFLAGS = $(INC) -m32 -DLINUX_BUILD -ggdb -D_WEBSOCKETPP_CPP11_STL_
LDFLAGS = $(LIB) -shared

ifeq ($(shell uname -s), Darwin)
	CXX = c++ -std=gnu++0x -stdlib=libstdc++
	CFLAGS += -Wno-tautological-compare
	LDFLAGS += -framework OpenGL -lcrypto -lssl -mmacosx-version-min=10.6
else
	CXX = clang++ -std=c++11
	CC = clang
	CFLAGS += -fdiagnostics-color=auto -Wno-tautological-compare
	LDFLAGS +=
endif


all: $(OUT)

uvwslay.o: uvwslay/uvwslay.c
	$(CC) $(CFLAGS) uvwslay/uvwslay.c -c -o uvwslay.o

$(OUT): $(SRC) $(DEP)
	-@mkdir -p `dirname $(OUT)`
	$(CXX) $(SRC) $(DEP) -o $(OUT) -DDFHACK_VERSION=\"$(DFHACKVER)-$(DFHACKREL)\" $(CFLAGS) $(LDFLAGS)

install: $(OUT)
	cp $(OUT) "$(DF)/hack/plugins/"

clean:
	-rm $(OUT) uvwslay.o
