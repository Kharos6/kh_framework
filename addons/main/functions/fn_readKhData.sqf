params [["_file", "", [""]], ["_variable", "", [""]], ["_defaultValue", nil]];

if ((_file isEqualTo "") || (_variable isEqualTo "")) exitWith {
	false;
};

private _slices = (parseNumber (("kh_framework" callExtension ["SliceData", [_this select 0, _this select 1]]) select 0)) - 1;
private _output = [];

for "_i" from 0 to _slices do {
	_output pushBack (("kh_framework" callExtension ["ReadKHData", [_file, _variable, _i]]) select 0);
};

private _parsedOutput = _output joinString "";

if ("KH_ERROR: " in _parsedOutput) exitWith {
	_defaultValue;
};

_output = parseSimpleArray _parsedOutput;

switch (_output select 0) do {
	case "ARRAY";
	case "STRING";
	case "BOOL";
	case "SCALAR": {
		_output select 1;
	};

	case "CODE": {
		compile (_output select 1);
	};

	case "HASHMAP": {
		createHashMapFromArray (_output select 1);
	};

	case "TEXT": {
		text (_output select 1);
	};

	default {
		_defaultValue;
	};
};