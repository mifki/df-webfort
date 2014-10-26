DFHACKVER ?= 0.34.11
DFHACKREL ?= r5

DF ?= df_linux
DH ?= dfhack

NOPOLL_DIR ?= nopoll-0.2.6.b130
NOPOLL_I ?= $(NOPOLL_DIR)/src
NOPOLL_L ?= $(NOPOLL_DIR)/src/.libs/libnopoll.a

LIBUV_DIR ?= libuv-1.0.0-rc2
LIBUV_I ?= $(LIBUV_DIR)/include
LIBUV_L ?= $(LIBUV_DIR)/.libs/libuv.a

SRC = server.cpp webfort.cpp $(LIBUV_L) $(NOPOLL_L)
DEP =
OUT = dist/dfhack-$(DFHACKREL)/webfort.plug.so

SDL_I = $(shell sdl-config --cflags)

INC = -I"$(DH)/library/include" -I"$(DH)/library/proto" -I"$(DH)/depends/protobuf" -I"$(DH)/depends/tthread" -I$(LIBUV_I) -I$(NOPOLL_I) $(SDL_I)
LIB = -L"$(DH)/build/library" -ldfhack -L"$(DH)/build/depends/tthread" -ldfhack-tinythread -L"/usr/lib/i386-linux-gnu"

CFLAGS = $(INC) -m32 -DLINUX_BUILD -ggdb
LDFLAGS = $(LIB) -shared

ifeq ($(shell uname -s), Darwin)
	CXX = c++ -std=gnu++0x -stdlib=libstdc++
	CFLAGS += -Wno-tautological-compare
	LDFLAGS += -framework OpenGL -lcrypto -lssl -mmacosx-version-min=10.6
else
	CXX = clang++ -std=gnu++0x
	CFLAGS += -fdiagnostics-color=auto -Wno-tautological-compare
	LDFLAGS += -lcrypto -lssl
endif


all: $(OUT)

$(OUT): $(SRC) $(DEP)
	-@mkdir -p `dirname $(OUT)`
	$(CXX) $(SRC) -o $(OUT) -DDFHACK_VERSION=\"$(DFHACKVER)-$(DFHACKREL)\" $(CFLAGS) $(LDFLAGS)

install: $(OUT)
	cp $(OUT) "$(DF)/hack/plugins/"

clean:
	-rm $(OUT)
