## HOW TO COMPILE ##
Unfortunately, Webfort does not have a solid build process in place (issue #3).

Until it does, use this page to document any holistic steps taken for
later.

### Ubuntu 14.04 LTS, 64-bit ###
a quick recording of what it took to compile and run webfort on my
Ubuntu 14.04 machine, 64bit. Fair warning: this will take a significant
chunk of time.

webfort depends on some unpackaged dependencies:

* Dwarf Fortress 0.34.11 (download a fresh copy, just for webfort)

* dfhack 0.34.11-r5

* SDL 1.2 (just the headers)

* nopoll 0.2.6.b130

All of these need to be 32-bit, because Dwarf Fortress is 32-bit. We'll
install all of them in a 32-bit lxc container, because Ubuntu's
multiarch support isn't so great.

First step, make the container.

	# lxc-create -t ubuntu -n dfhack -- --bindhome <USERNAME> -a i386

boot into it, using the credentials of your current user

	# lxc-start -n dfhack


then install all this

	# apt-get install curl git build-essential cmake libsdl1.2-dev \
	 libsdl-image1.2 libsdl-ttf2.0-0 libgtk2.0-0 libjpeg62 libglu1-mesa \
	 libopenal1 libxml-libxml-perl libxml-libxslt-perl libssl-dev

then get Dwarf Fortress. DF needs an extra spriteset and init to be
properly configured, but I didn't do that at the time of writing so add
your own steps.

	$ curl http://www.bay12games.com/dwarves/df_34_11_linux.tar.bz2 | tar jxf -

then build dfhack

	$ git clone http://github.com/DFHack/dfhack.git
	$ cd dfhack
	$ git checkout 0.34.11-r5
	$ git submodule update --init --recursive
	$ mkdir build && cd build
	$ cmake .. -DCMAKE_BUILD_TYPE:string=Release -DCMAKE_INSTALL_PREFIX=$PWD/../../df_linux
	$ make && make install
	$ cd ../..

then build nopoll

	$ curl https://no-poll.googlecode.com/files/nopoll-0.2.6.b130.tar.gz |
	tar zxf -
	$ cd nopoll-0.2.6.b130
	$ ./configure
	$ make
	$ cd ..

then build webfort.

	$ make
	$ make install

and then try running dfhack from your new df install. chances are you
come across this:
https://github.com/DFHack/dfhack/blob/master/Compile.rst#fixing-the-libstdc-version-bug
So just follor the fix there

#### websocketpp branch ####
you'll also need

* libboost_system
* libasio
* websocketpp (already a submodule)
* not nopoll

TODO. I, personally, used my package manager for boost and asio.


### Windows 7, VS2010 ###

TODO
