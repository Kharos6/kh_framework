params ["_file", "_variable", ["_defaultValue", nil]];
private _slices = (parseNumber (("kh_framework" callExtension ["SliceData", [_this select 0, _this select 1]]) select 0)) - 1;
private _output = [];

for "_i" from 0 to _slices do {
	_output pushBack (("kh_framework" callExtension ["ReadKHData", [_file, _variable, _i]]) select 0);
};

_output joinString "";

if ("KH_ERROR: " in _output) exitWith {
	_defaultValue;
};

private _output = parseSimpleArray _output;

switch (_output select 0) do {
	case "ARRAY": {
		parseSimpleArray (_output select 1);
	};

	case "STRING": {
		_output select 1;
	};

	case "SCALAR": {
		parseNumber (_output select 1);
	};

	case "HASHMAP": {
		createHashMapFromArray (parseSimpleArray (_output select 1));
	};

	case "BOOL": {
		if ((_output select 1) isEqualTo "true") then {
			true;
		}
		else {
			false;
		};
	};

	case "CODE": {
		compile (_output select 1);
	};

	case "TEXT": {
		text (_output select 1);
	};

	default {
		_defaultValue;
	};
};