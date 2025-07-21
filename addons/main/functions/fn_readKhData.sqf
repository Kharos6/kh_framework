params [["_file", "", [""]], ["_variable", "", [""]], ["_defaultValue", nil]];

if ((_file isEqualTo "") || (_variable isEqualTo "")) exitWith {
	nil;
};

("kh_framework" callExtension ["SliceKHData", _this select [0, 2]]) params ["_slices", "_returnCode"];

if ([_returnCode] call KH_fnc_parseBoolean) exitWith {
	diag_log (text ([_slices, " | EXTENSION = kh_framework | FUNCTION = SliceKHData | ARGUMENTS = ", _this select [0, 2]] joinString ""));
	_defaultValue;
};

_slices = (parseNumber _slices) - 1;
private _result = [];
private _break = false;

for "_i" from 0 to _slices do {
	("kh_framework" callExtension ["ReadKHData", [_file, _variable, _i]]) params ["_currentResult", "_returnCode"];

	if ([_returnCode] call KH_fnc_parseBoolean) then {
		diag_log (text ([_currentResult, " | EXTENSION = kh_framework | FUNCTION = ReadKHData | ARGUMENTS = ", [_file, _variable, _i]] joinString ""));
		_break = true;
		break;
	};

	_result pushBack _currentResult;
};

if _break exitWith {
	_defaultValue;
};

_result = parseSimpleArray (_result joinString "");

switch (_result select 0) do {
	case "ARRAY";
	case "STRING";
	case "BOOL";
	case "SCALAR": {
		_result select 1;
	};

	case "CODE": {
		if ((([(_result select 1)] joinString "") select [0, 1]) isEqualTo "{") then {
			diag_log (text (["KH_ERROR: BAD CODE TYPE", " | EXTENSION = kh_framework | FUNCTION = ReadKHData | ARGUMENTS = ", [_file, _variable]] joinString ""));
			_defaultValue;
		}
		else {
			compile (_result select 1);
		};
	};

	case "HASHMAP": {
		createHashMapFromArray (_result select 1);
	};

	case "TEXT": {
		text (_result select 1);
	};

	default {
		_defaultValue;
	};
};