## HOW TO COMPILE ##

Webfort currently compiles as a submodule of DFHack, much like
stonesense.

It requires only minimal changes to the default DFHack release, which
can be found at <https://github.com/Alloyed/dfhack/tree/webfort>.

To get a working copy:

	git clone https://github.com/Alloyed/dfhack
	git checkout webfort
	git submodule update --init --recursive

Then you can follow the usual
[DFHack compiling process](https://github.com/Alloyed/dfhack/blob/webfort/Compile.rst).

Below are extra notes which you should probably read.

### Windows 7 VS2010 ###

On Windows, all Webfort dependencies are automatically installed by
downloading and extracting this archive:

https://s3.amazonaws.com/webfort/webfort-deps-r1.tar.gz

This includes a number of Boost binaries, as well as SDL headers.

### Ubuntu 14.04 LTS 64-bit ###

In my experience, Multilib support on Debian/Ubuntu is bad enough to
make the linux container option suggested by upstream DFHack to be the
only sane option.

Get a container by executing:

	# lxc-create -t ubuntu -n dfhack -- --bindhome <USERNAME> -a i386

Where ```<USERNAME>``` is your username, and ubuntu is whichever distro you'd
like. Then boot into it, using the credentials of that user:

	# lxc-start -n dfhack

Once there you can install the necessary dependencies for webfort:

	# apt-get install libboost-system-dev libboost-regex-dev libasio-dev libsdl1.2-dev

In addition to everything you need for dfhack:

	# apt-get install curl git build-essential cmake libxml-libxml-perl libxml-libxslt-perl

Then follow the usual instructions.
