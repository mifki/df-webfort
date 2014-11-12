#!/bin/sh
# Build script for windows, written in unix :P
# This file is a part of Web Fortress
# (c) 2014 Kyle Mclamb <alloyed@tfwno.gf>

if [ ! -r "$1" ]; then
	echo "Invalid file: $1"
	echo "Usage: $0 <Path to webfort.plug.dll>"
	exit 1
fi

rm -rf package
mkdir -v package
mkdir -vp package/hack/plugins

cp -v "$1" package/hack/plugins/
cp -vr dist/* package/
cp -vr static package/web

cp_prefixed() {
	cp -v $1 package/WF-$1
}

cp_prefixed README.md
cp_prefixed INSTALLING.txt
cp_prefixed LICENSE

zipname="webfort-$(git describe --tag).zip"

rm -v "$zipname"
(cd package && zip -r "../$zipname" ./*)

rm -rf package
echo "$zipname: Done."
