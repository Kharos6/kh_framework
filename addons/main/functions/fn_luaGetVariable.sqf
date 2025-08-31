params [["_variable", "", [""]], "_defaultValue"];
("kh_framework" callExtension ["LuaGetVariable", [_variable]]) params ["_currentResult", "_returnCode"];

if ([_returnCode] call KH_fnc_parseBoolean) exitWith {
	diag_log (text ([_slices, " | EXTENSION = kh_framework | FUNCTION = LuaGetVariable | ARGUMENTS = ", [_variable]] joinString ""));
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

	case "HASHMAP": {
		createHashMapFromArray (_result select 1);
	};

	default {
		_defaultValue;
	};
};