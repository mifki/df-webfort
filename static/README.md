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

And an example of using them:

	http://myhost/webfort.html?host=theirhost&port=80&hide-chat=true

Parameters can be stored into your browser's localStorage, where they can
persist between sessions. for example, opening:

	http://myhost/webfort.html?nick=myNick&store=true

will store the nick `myNick` and restore it such that

	http://myhost/webfort.html

will also have the the nick `myNick`. ATM, storage can only be reset to
defaults by using the console command:

	localStorage.clear()
