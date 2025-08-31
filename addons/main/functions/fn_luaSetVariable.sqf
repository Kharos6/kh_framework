params [["_variable", "", [""]], "_value", ["_type", true, [true, ""]]];

if (_type isEqualTo true) then {
	_type = typeName _value;
};

if (
	(_type isNotEqualTo "ARRAY") && 
	(_type isNotEqualTo "STRING") && 
	(_type isNotEqualTo "SCALAR") && 
	(_type isNotEqualTo "HASHMAP") && 
	(_type isNotEqualTo "BOOL")
   ) exitWith {
	diag_log (text (["KH_ERROR: BAD DATA TYPE", " | EXTENSION = kh_framework | FUNCTION = LuaSetVariable | ARGUMENTS = ", _this] joinString ""));
};

("kh_framework" callExtension ["LuaSetVariable", _this]) params ["_result", "_returnCode"];

if ([_returnCode] call KH_fnc_parseBoolean) then {
	diag_log (text ([_result, " | EXTENSION = kh_framework | FUNCTION = LuaSetVariable | ARGUMENTS = ", _this] joinString ""));
};