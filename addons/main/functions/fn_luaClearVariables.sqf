("kh_framework" callExtension "LuaClearVariables") params ["_result", "_returnCode"];

if ([_returnCode] call KH_fnc_parseBoolean) then {
	diag_log (text ([_result, " | EXTENSION = kh_framework | FUNCTION = LuaClearVariables | ARGUMENTS = ", []] joinString ""));
	nil;
}
else {
	true;
};