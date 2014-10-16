// Insert your own here.
var iframeURL = "http://webchat.quakenet.org/?randomnick=1&channels=webfortress&prompt=1&uio=MT1mYWxzZSYxNj10cnVlJjEzPWZhbHNlJjE0PWZhbHNl1d";

function fitWebchatToWindow() {
	document.getElementById('webchat').height = (window.innerHeight - 20) & (~15);
}

function insertWebchat() {
	console.log("WEEB CHAT");
	var frame = document.createElement('iframe');
	frame.setAttribute('id', 'webchat');
	frame.setAttribute('src', iframeURL);
	document.getElementById('webchat-container').appendChild(frame);
}

var oldResize = window.onresize;
window.onresize =  function() {
	fitWebchatToWindow();
	oldResize();
}

window.onload = function() {
	insertWebchat();
	fitWebchatToWindow();
}
