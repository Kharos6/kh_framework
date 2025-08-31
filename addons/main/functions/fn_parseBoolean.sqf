params ["_value", ["_returnInteger", false, [true]]];

private _return = if (isNil "_value") then {
	false;
}
else {
	switch _value do {
		case false: {
			false;
		};

		case 0: {
			false;
		};

		case "FALSE": {
			false;
		};

		case "false": {
			false;
		};

		case "0": {
			false;
		};

		default {
			true;
		};
	};
};

if !_returnInteger then {
	_return;
}
else {
	parseNumber _return;
};