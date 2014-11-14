function getJsonFromUrl() {
	var query = location.search.substr(1);
	var result = {};
	var stored = localStorage.getItem("settings");
	if (stored) {
		result = JSON.parse(stored);
	}
	query.split("&").forEach(function(part) {
		var item = part.split("=");
		var key = item[0].replace('-', '_');
		var val = item[1];

		if (key !== 'nick') { // we want raw nicks
			val = decodeURIComponent(val);
			if (val === "false") {
				val = false;
			}
		}
		result[key] = val;
	});
	return result;
}

var params = getJsonFromUrl();
console.log(JSON.stringify(params, null, 4));

if (params.store) {
	delete params.store;
	localStorage.setItem('settings', JSON.stringify(params));
}

