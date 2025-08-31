params [["_function", "", [""]], ["_name", "", [""]]];
if !(isNil {KH_var_cachedLuaFunctions get _name}) exitWith {};

if (".lua" in _function) then {
	_function = preprocessFile _function;
};

("kh_framework" callExtension ["LuaCompile", _this]) params ["_result", "_returnCode"];

if ([_returnCode] call KH_fnc_parseBoolean) then {
	diag_log (text ([_result, " | EXTENSION = kh_framework | FUNCTION = LuaCompile | ARGUMENTS = ", _this] joinString ""));
}
else {
	missionNamespace setVariable [_name, _function];
	KH_var_cachedLuaFunctions set [_name, _function];
	nil;
};