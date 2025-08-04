("kh_framework" callExtension "LuaResetState") params ["_result", "_returnCode"];

if ([_returnCode] call KH_fnc_parseBoolean) then {
	diag_log (text ([_result, " | EXTENSION = kh_framework | FUNCTION = LuaResetState | ARGUMENTS = ", []] joinString ""));
	nil;
}
else {
	true;
};