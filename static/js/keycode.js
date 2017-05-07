var _keyCodes = {
	0: "\\",
	8: "backspace",
	9: "tab",
	12: "num",
	13: "enter",
	16: "shift",
	17: "ctrl",
	18: "alt",
	19: "pause",
	20: "caps",
	27: "esc",
	32: "space",
	33: "pageup",
	34: "pagedown",
	35: "end",
	36: "home",
	37: "left",
	38: "up",
	39: "right",
	40: "down",
	44: "print",
	45: "insert",
	46: "delete",
	48: "0",
	49: "1",
	50: "2",
	51: "3",
	52: "4",
	53: "5",
	54: "6",
	55: "7",
	56: "8",
	57: "9",
	65: "a",
	66: "b",
	67: "c",
	68: "d",
	69: "e",
	70: "f",
	71: "g",
	72: "h",
	73: "i",
	74: "j",
	75: "k",
	76: "l",
	77: "m",
	78: "n",
	79: "o",
	80: "p",
	81: "q",
	82: "r",
	83: "s",
	84: "t",
	85: "u",
	86: "v",
	87: "w",
	88: "x",
	89: "y",
	90: "z",
	91: "cmd",
	92: "cmd",
	93: "cmd",
	96: "num_0",
	97: "num_1",
	98: "num_2",
	99: "num_3",
	100: "num_4",
	101: "num_5",
	102: "num_6",
	103: "num_7",
	104: "num_8",
	105: "num_9",
	106: "num_multiply",
	107: "num_add",
	108: "num_enter",
	109: "num_subtract",
	110: "num_decimal",
	111: "num_divide",
	124: "print",
	144: "num",
	145: "scroll",
	186: ";",
	187: "=",
	188: ",",
	189: "-",
	190: ".",
	191: "/",
	192: "`",
	219: "[",
	220: "\\",
	221: "]",
	222: "\'",
	223: "`",
	224: "cmd",
	225: "alt",
	57392: "ctrl",
	63289: "num",
	59: ";"
};

function logKeyCode(ev) {
	var s = "";
	if (ev.ctrlKey) {
		s += "CTRL-";
	}
	if (ev.altKey) {
		s += "ALT-";
	}
	if (ev.shiftKey) {
		s += "SHIFT-";
	}
	s += _keyCodes[ev.keyCode];
	console.log(s + " pressed.");
}

function logCharCode(ev) {
	var s = "";
	if (ev.ctrlKey) {
		s += "CTRL-";
	}
	if (ev.altKey) {
		s += "ALT-";
	}
	if (ev.shiftKey) {
		s += "SHIFT-";
	}
	s += String.fromCharCode(ev.which);
	console.log(s + " pressed.");
}
