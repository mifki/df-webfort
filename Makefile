DF=/Users/vit/Desktop/Macnewbie/Dwarf\ Fortress

INC=-I/Users/vit/Downloads/dfhack/library/include -I/Users/vit/Downloads/dfhack/library/proto -I/Users/vit/Downloads/dfhack/depends/protobuf -I/Users/vit/Downloads/dfhack/depends/tthread -I/Users/vit/Downloads/dfhack/depends/lua/include -I/Users/vit/Downloads/nopoll-0.2.6.b130/src
LIB=-L$(DF)/hack -ldfhack -L/Users/vit/Downloads/dfhack/build/depends/tthread -ldfhack-tinythread /Users/vit/Downloads/nopoll-0.2.6.b130/src/.libs/libnopoll.a -lssl -lcrypto -F$(DF)/libs -framework SDL

all: twbt.plug.so

inst: twbt.plug.so
	cp twbt.plug.so $(DF)/hack/plugins

twbt.plug.so: twbt.cpp
	g++ twbt.cpp -o twbt.plug.so -m32 -shared -std=gnu++11 -stdlib=libstdc++ $(INC) $(LIB) -DDFHACK_VERSION=\"0.34.11-r3\" -Wno-ignored-attributes -Wno-tautological-compare