/*
 * webfort.js
 * Copyright (c) 2014 mifki, ISC license.
 */

/*jslint browser:true */

var params = getParams();
// TODO: tag colors
var colors = [
	32, 39, 49,
	0, 106, 255,
	68, 184, 57,
	114, 156, 251,
	212, 54, 85,
	176, 50, 255,
	217, 118, 65,
	170, 196, 178,
	128, 151, 156,
	48, 165, 255,
	144, 255, 79,
	168, 212, 255,
	255, 82, 82,
	255, 107, 255,
	255, 232, 102,
	255, 250, 232
];

var MAX_FPS = 20;

var host = params.host || document.location.hostname;
var port = params.port || '1234';
var tileSet = params.tiles || "Spacefox_16x16.png";
var textSet = params.text  || "ShizzleClean.png";
var colorscheme = params.colors || undefined;
var nick = params.nick || "";

var wsUri = 'ws://' + host + ':' + port + '/' + encodeURIComponent(nick);
var active = false;
var lastFrame = 0;

// Converts integer value in seconds to a time string, HH:MM:SS
function toTime(n) {
	var h = Math.floor(n / 60  / 60);
	var m = Math.floor(n / 60) % 60;
	var s = n % 60;
	return ("0" + h).slice(-2) + ":" +
	       ("0" + m).slice(-2) + ":" +
	       ("0" + s).slice(-2);
}

function plural(n, unit)
{
	if (n === 1) {
		return n + " " + unit;
	}
	return n + " " + unit + "s";
}

// Converts an integer value in ticks to the dwarven calendar
function toGameTime(n) {
	var years = Math.floor(n / 12 / 28 / 1200);
	var months = Math.floor(n / 28 / 1200) % 12;
	var days = Math.floor(n / 1200) % 28;
	var ticks = n % 1200;

	var times = [];
	if (years > 0) {
		times.push(plural(years, "year"));
	}
	if (months > 0) {
		times.push(plural(months, "month"));
	} else if (days > 0) {
		times.push(plural(days, "day"));
	} else {
		times.push(plural(ticks, "tick"));
	}
	
	return times.join(", ");
}

function setStats(userCount, ingame_time, timeLeft) {
	var u = document.getElementById('user-count');
	var t = document.getElementById('time-left');
	u.innerHTML = String(userCount) + " <i class='fa fa-users'></i>";

	if (timeLeft === -1) {
		t.innerHTML = "";
	} else {
		t.innerHTML = (ingame_time? toGameTime(timeLeft) : toTime(timeLeft)) +
			" <i class='fa fa-clock-o'></i>";
	}
}

function setStatus(text, color, onclick) {
	var m = document.getElementById('message');
	m.innerHTML = text;
	var st = m.parentNode;
	if (onclick) {
		st.addEventListener('click', onclick);
	}
	st.style.backgroundColor = color;
}

function connect() {
	setStatus('Connecting...', 'orange');
	websocket = new WebSocket(wsUri, ['WebFortress-v2.0', 'WebFortress-invalid']);
	websocket.binaryType = 'arraybuffer';
	websocket.onopen  = onOpen;
	websocket.onclose = onClose;
	websocket.onerror = onError;
}

function onOpen(evt) {
	setStatus('Connected', 'orange');

	websocket.send(new Uint8Array([115]));

	websocket.send(new Uint8Array([110]));
	websocket.onmessage = onMessage;
}

var isError = false;
function onClose(evt) {
	console.log("Disconnect code #" + evt.code + ", reason: " + evt.reason);
	console.log(isError);
	if (isError) {
		isError = false;
		setStatus('Connection Error. Click to retry', 'red', connect);
	} else if (evt.reason) {
		setStatus(evt.reason + ' Click to try again.', 'red', connect);
	} else {
		setStatus('Unknown disconnect: Check the console (Ctrl-Shift-J), then click to reconnect.', 'red', connect);
	}
}

function onError(ev) {
	console.log("error triggered.");
	isError = true;
}

function requestTurn() {
	websocket.send(new Uint8Array([116]));
}

function renderQueueStatus(s) {
	if (s.isActive) {
		active = true;
		setStatus("You're in charge now! Click here to end your turn.", 'green', requestTurn);
	} else if (s.isNoPlayer) {
		setStatus("Nobody is playing right now. Click here to ask for a turn.", 'grey', requestTurn);
	} else {
		var displayedName = s.currentPlayer || "Somebody else";
		setStatus(displayedName +" is doing their best. Please wait warmly.", 'orange');
	}
	setStats(s.playerCount, s.ingameTime, s.timeLeft);
}

function renderUpdate(ctx, data, offset) {
	var t = [];
	var k;
	var x;
	var y;
	var s;
	var bg;
	var fg;
	for (k = offset; k < data.length; k += 5) {
		x = data[k + 0];
		y = data[k + 1];

		s = data[k + 2];
		bg = data[k + 3] % 16;
		fg = data[k + 4];

		var bg_x = ((bg % 4) * 256) + 15 * 16;
		var bg_y = (Math.floor(bg / 4) * 256) + 15 * 16;
		ctx.drawImage(cd, bg_x, bg_y, 16, 16, x * 16, y * 16, 16, 16);

		if (data[k + 3] & 64) {
			t.push(k);
			continue;
		}
		var fg_x = (s % 16) * 16 + ((fg % 4) * 256);
		var fg_y = Math.floor(s / 16) * 16 + (Math.floor(fg / 4) * 256);
		ctx.drawImage(cd, fg_x, fg_y, 16, 16, x * 16, y * 16, 16, 16);
	}

	for (var m = 0; m < t.length; m++) {
		k = t[m];
		x = data[k + 0];
		y = data[k + 1];

		s = data[k + 2];
		bg = data[k + 3];
		fg = data[k + 4];

		var i = (s % 16) * 16 + ((fg % 4) * 256);
		var j = Math.floor(s / 16) * 16 + (Math.floor(fg / 4) * 256);
		ctx.drawImage(ct, i, j, 16, 16, x * 16, y * 16, 16, 16);
	}
}

function onMessage(evt) {
	var data = new Uint8Array(evt.data);

	var ctx = canvas.getContext('2d');
	if (data[0] === 110) {
		if (stats) { stats.begin(); }
		var gameStatus = {};
		gameStatus.playerCount = data[1] & 127;

		gameStatus.isActive   = (data[2] & 1) !== 0;
		gameStatus.isNoPlayer = (data[2] & 2) !== 0;
		gameStatus.ingameTime = (data[2] & 4) !== 0;
		console.log(gameStatus);

		gameStatus.timeLeft =
			(data[3]<<0) |
			(data[4]<<8) |
			(data[5]<<16) |
			(data[6]<<24);

		// FIXME: we shouldn't need resize data
		var neww = data[7] * 16;
		var newh = data[8] * 16;

		var nickSize = data[9];
		// this only works because we know the input is uri-encoded ascii
		var activeNick = "";
		for (var i = 10; (i < 10 + nickSize) && data[i] !== 0; i++) {
			activeNick += String.fromCharCode(data[i]);
		}
		gameStatus.currentPlayer = decodeURIComponent(activeNick);

		renderQueueStatus(gameStatus);
		renderUpdate(ctx, data, nickSize+10);

		var now = performance.now();
		var nextFrame = (1000 / MAX_FPS) - (now - lastFrame);
		if (nextFrame < 4) {
			websocket.send(new Uint8Array([110]));
		} else {
			setTimeout(function() {
				websocket.send(new Uint8Array([110]));
			}, nextFrame);
		}
		lastFrame = performance.now();
		if (stats) { stats.end(); }
	}
}

function colorize(img, cnv) {
	var ctx3 = cnv.getContext('2d');

	for (var j = 0; j < 4; j++) {
		for (var i = 0; i < 4; i++) {
			var c = j * 4 + i;

			ctx3.drawImage(img, i * 256, j * 256);

			var idata = ctx3.getImageData(i * 256, j * 256, 256, 256);
			var pixels = idata.data;

			for (var u = 0, len = pixels.length; u < len; u += 4) {
				pixels[u] = pixels[u] * (colors[c * 3 + 0] / 255);
				pixels[u + 1] = pixels[u + 1] * (colors[c * 3 + 1] / 255);
				pixels[u + 2] = pixels[u + 2] * (colors[c * 3 + 2] / 255);
			}
			ctx3.putImageData(idata, i * 256, j * 256);

			ctx3.fillStyle = 'rgb(' +
					colors[c * 3 + 0] + ',' +
					colors[c * 3 + 1] + ',' +
					colors[c * 3 + 2] + ')';

			ctx3.fillRect(i * 256 + 16 * 15, j * 256 + 16 * 15, 16, 16);
		}
	}
}

var loading = 0;
function make_loader() {
	loading += 1;
	return function() {
		loading -= 1;
		init();
	};
}

var cd, ct;
function init() {
	if (loading > 0)
		return;

	document.body.style.backgroundColor =
		'rgb(' + colors[0] + ',' + colors[1] + ',' + colors[2] + ')';

	cd = document.createElement('canvas');
	cd.width = cd.height = 1024;
	colorize(ts, cd);

	ct = document.createElement('canvas');
	ct.width = ct.height = 1024;
	colorize(tt, ct);

	lastFrame = performance.now();

	connect();
}

var stats;
if (params.show_fps) {
	stats = new Stats();
	document.body.appendChild(stats.domElement);
	stats.domElement.style.position = "absolute";
	stats.domElement.style.bottom = "0";
	stats.domElement.style.left   = "0";
}

function getFolder(path) {
	return path.substring(0, path.lastIndexOf('/') + 1);
}

var root = getFolder(window.location.pathname);

var ts = document.createElement('img');
ts.src =  root + "art/" + tileSet;
ts.onload = make_loader();

var tt = document.createElement('img');
tt.src = root + "art/" + textSet;
tt.onload = make_loader();

if (colorscheme !== undefined) {
	var colorReq = new XMLHttpRequest();
	var colorLoader = make_loader();
	colorReq.onload = function() {
		colors = JSON.parse(this.responseText);
		colorLoader();
	};
	colorReq.open("get", root + "colors/" + colorscheme);
	colorReq.send();
}


var canvas = document.getElementById('myCanvas');

document.onkeydown = function(ev) {
	if (!active)
		return;

	if (ev.keyCode === 91 ||
	    ev.keyCode === 18 ||
	    ev.keyCode === 17 ||
	    ev.keyCode === 16) {
		return;
	}

	if (ev.keyCode < 65) {
		var mod = (ev.shiftKey << 1) | (ev.ctrlKey << 2) | ev.altKey;
		var data = new Uint8Array([111, ev.keyCode, 0, mod]);
		logKeyCode(ev);
		websocket.send(data);
		ev.preventDefault();
	} else {
		lastKeyCode = ev.keyCode;
	}
};

document.onkeypress = function(ev) {
	if (!active)
		return;

	var mod = (ev.shiftKey << 1) | (ev.ctrlKey << 2) | ev.altKey;
	var data = new Uint8Array([111, 0, ev.charCode, mod]);
	logCharCode(ev);
	websocket.send(data);

	ev.preventDefault();
};


function fitCanvasToParent() {
	var maxw = canvas.parentNode.offsetWidth;
	var maxh = canvas.parentNode.offsetHeight;
	var aspectRatio = canvas.width / canvas.height;

	if (maxw / maxh < aspectRatio) {
		canvas.style.width  = maxw + 'px';
		canvas.style.height = "";
	} else {
		canvas.style.width  = "";
		canvas.style.height = maxh + 'px';
	}
}

window.onresize = fitCanvasToParent;
window.onload   = fitCanvasToParent;
