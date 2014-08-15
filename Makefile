DFHACKVER ?= 0.34.11
DFHACKREL ?= r3

DF ?= /Users/vit/Downloads/Macnewbie/Dwarf Fortress
DH ?= /Users/vit/Downloads/dfhack-$(DFHACKREL)

NOPOLL_I ?= nopoll/src
NOPOLL_L ?= nopoll/src/.libs

SRC = webfort.cpp
DEP = 
OUT = dist/dfhack-$(DFHACKREL)/webfort.plug.so

INC = -I"$(DH)/library/include" -I"$(DH)/library/proto" -I"$(DH)/depends/protobuf" -I"$(DH)/depends/tthread" -I"$(DH)/depends/lua/include" -I$(NOPOLL_I)
LIB = -L"$(DH)/build/library" -ldfhack -L"$(DH)/build/depends/tthread" -ldfhack-tinythread -lSDL $(NOPOLL_L)/libnopoll.a

CFLAGS = $(INC) -m32 -DLINUX_BUILD -O3
LDFLAGS = $(LIB) -shared 

ifeq ($(shell uname -s), Darwin)
	CXX = c++ -std=gnu++0x -stdlib=libstdc++
	CFLAGS += -Wno-tautological-compare
	LDFLAGS += -framework OpenGL -lcrypto -lssl -mmacosx-version-min=10.6
else
	CXX = c++ -std=c++0x
endif


all: $(OUT)

$(OUT): $(SRC) $(DEP)
	-@mkdir -p `dirname $(OUT)`
	pwd
	$(CXX) $(SRC) -o $(OUT) -DDFHACK_VERSION=\"$(DFHACKVER)-$(DFHACKREL)\" $(CFLAGS) $(LDFLAGS)

inst: $(OUT)
	cp $(OUT) "$(DF)/hack/plugins/"

clean:
	-rm $(OUT)
