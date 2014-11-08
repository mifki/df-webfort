
// The IRC channel to connect to. On quakenet, by default.
var channelName = "webfortress";

var nickSetting = "nick=Dwarf_.";
if (nick) {
	nickSetting = "nick=" + nick;
}

var iframeURL = "http://webchat.quakenet.org/?" + 
	"channels=webfortress&" + 
	"prompt=1&" +
	"uio=MT1mYWxzZSYxNj10cnVlJjEzPWZhbHNlJjE0PWZhbHNl1d&" +
	nickSetting;

var _onload = window.onload
window.onload = function() {
	_onload ? _onload() : null;
	var frame = document.createElement('iframe');
	frame.setAttribute('id', 'webchat');
	frame.setAttribute('src', iframeURL);
	document.getElementById('webchat-container').appendChild(frame);
}

function elem_id(id) { return document.getElementById(id) }
if (params.hide_chat) {
	elem_id("game-container").classList.add("hide-chat");
	elem_id("webchat-container").classList.add("hide-chat");
}
