params [["_variable", "", [""]]];
("kh_framework" callExtension ["LuaDeleteVariable", _this]) params ["_result", "_returnCode"];

if ([_returnCode] call KH_fnc_parseBoolean) then {
	diag_log (text ([_result, " | EXTENSION = kh_framework | FUNCTION = LuaDeleteVariable | ARGUMENTS = ", _this] joinString ""));
};