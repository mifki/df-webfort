Parameters
==========

webfort.html supports a number of query parameters for configuration. Some
of these really should be persistent settings but others are really only
for debug purposes. here are all of them:

| option      | value               | default              | description                               |
|-------------|---------------------|----------------------|-------------------------------------------|
| `host`      | any hostname        | current hostname     | The websocket domain to connect to.       |
| `port`      | any port number     | 1234                 | The port number of the websocket.         |
| `tiles`     | an image in `art/`  | `Spacefox_16x16.png` | The tileset to use.                       |
| `text`      | an image in `art/`  | `ShizzleClean.png`   | the tileset to use for ingame text.       |
| `show-fps`  | a boolean           | false                | Whether or not to show the FPS counter.   |
| `hide-chat` | a boolean           | false                | Whether or not to hide the IRC side pane. |
| `colors`    | a file in `colors/` | default colorscheme  | The colorscheme to use, in json format.   |
| `nick`      | any string          | random               | The nickname to use                       |
| `store`     | a boolean           | undefined            | if true, store all current settings       |

A quick primer on query strings:

Anything past a ? in a URL is a query string

	http://<your host>/webfort.html?param=value

here, the parameter `param` is being set to `value`.

	http://<your host>/webfort.html?param

If you don't give a value. it is assumed to be true. So here,
`param` is true.

You can also chain multiple parameters using &

	http://<your host>/webfort.html?param1=1&param2=2

Here, `param1` is set to `1`, and `param2` is set to `2`.

A real world example:

	http://<your host>/webfort.html?nick=Urist&hide-chat&tiles=ShizzleClean.png

Will set your `nick` to Urist, hide the chat pane, and set the tileset
to `ShizzleClean.png`.

Parameters can be stored into your browser's `localStorage`, where they can
persist between sessions. for example, opening:

	http://<your host>/webfort.html?nick=Urist&store

will store the nick `Urist` and restore it such that

	http://<your host>/webfort.html

will also have the the nick `Urist`. ATM, storage can only be reset to
defaults by using the console command:

	localStorage.clear()

