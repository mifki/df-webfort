// Insert your own here.
var iframeURL = "http://webchat.quakenet.org/?randomnick=1&channels=webfortress&prompt=1&uio=MT1mYWxzZSYxNj10cnVlJjEzPWZhbHNlJjE0PWZhbHNl1d";

var _onload = window.onload
window.onload = function() {
	_onload ? _onload() : null;
	var frame = document.createElement('iframe');
	frame.setAttribute('id', 'webchat');
	frame.setAttribute('src', iframeURL);
	document.getElementById('webchat-container').appendChild(frame);
}
