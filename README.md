## Web Fortress ##

This is a plugin for [Dwarf Fortress](http://bay12games.com) / [DFHack](http://github.com/dfhack/dfhack) that allows to play Dwarf Fortress remotely.

It also includes features provided by [Text Will Be Text](https://github.com/mifki/df-twbt) plugin (as of TWBT version 2.23).

### Installation ###

1. Install webfort plugin as usual.
2. Copy `ShizzleClean.png` or any other text font to `data/art` folder.
3. Copy all `.dll` files to your DF folder.
4. Ensure that `PRINT_MODE` is set to `STANDARD` in your `init.txt`, and set `FONT` to `ShizzleClean.png`.
5. Open `static/js/webchat.js` and edit the `iframeURL` variable to
   point to your preferred embeddable chat client. One possible choice
is [qwebirc](http://qwebirc.org).
6. Use any web server to serve files from `static` folder. You can use [Mongoose](http://cesanta.com/mongoose.shtml), just copy it to `static` folder and run.
7. Navigate to `http://<YOUR HOST>/webfort.html` and enjoy.

### Authors and Links ###

[Home Page / Sources](https://github.com/mifki/df-webfort) -- [Latest Release](https://github.com/mifki/df-webfort/releases) -- [Discussion](http://www.bay12forums.com/smf/index.php?topic=139167.0) -- [Report an Issue](https://github.com/mifki/df-webfort/issues)

Copyright (c) 2014, Vitaly Pronkin <pronvit@me.com>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
