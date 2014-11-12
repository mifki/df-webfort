#!/bin/sh
# Build script for windows, written in unix :P
rm -vrf package
mkdir -v package
mkdir -vp package/hack/plugins
cp -vr dist/* package/
cp -vr static package/web

cp_prefixed() {
	cp -v $1 package/WF-$1
}

cp_prefixed README.md
cp_prefixed INSTALLING.txt
cp_prefixed LICENSE

echo "Done. Now just drop webfort.plug.dll into package/hack/plugins/"
echo "and then zip -r webfort-$(git describe --tag) package/*"
