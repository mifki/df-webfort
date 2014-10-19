DFHACKVER ?= 0.34.11
DFHACKREL ?= r5

DF ?= df_linux
DH ?= dfhack

NOPOLL_DIR ?= nopoll-0.2.6.b130
NOPOLL_I ?= $(NOPOLL_DIR)/src
NOPOLL_L ?= $(NOPOLL_DIR)/src/.libs

SRC = webfort.cpp
DEP =
OUT = dist/dfhack-$(DFHACKREL)/webfort.plug.so

SDL_L = $(shell sdl-config --libs)
SDL_I = $(shell sdl-config --cflags)

INC = -I"$(DH)/library/include" -I"$(DH)/library/proto" -I"$(DH)/depends/protobuf" -I"$(DH)/depends/tthread" -I"$(DH)/depends/lua/include" -I$(NOPOLL_I) $(SDL_I)
LIB = -L"$(DH)/build/library" -ldfhack -L"$(DH)/build/depends/tthread" -ldfhack-tinythread -L"/usr/lib/i386-linux-gnu" $(SDL_L) $(NOPOLL_L)/libnopoll.a

CFLAGS = $(INC) -m32 -DLINUX_BUILD -ggdb
LDFLAGS = $(LIB) -shared

ifeq ($(shell uname -s), Darwin)
	CXX = c++ -std=gnu++0x -stdlib=libstdc++
	CFLAGS += -Wno-tautological-compare
	LDFLAGS += -framework OpenGL -lcrypto -lssl -mmacosx-version-min=10.6
else
	CXX = clang++ -std=gnu++0x
	CFLAGS += -fdiagnostics-color=auto
	LDFLAGS += -lcrypto -lssl
endif


all: $(OUT)

$(OUT): $(SRC) $(DEP)
	-@mkdir -p `dirname $(OUT)`
	pwd
	$(CXX) $(SRC) -o $(OUT) -DDFHACK_VERSION=\"$(DFHACKVER)-$(DFHACKREL)\" $(CFLAGS) $(LDFLAGS)

install: $(OUT)
	cp $(OUT) "$(DF)/hack/plugins/"

clean:
	-rm $(OUT)
