params [["_file", "", [""]]];

if (_file isEqualTo "") exitWith {
	nil;
};

("kh_framework" callExtension ["UnbinarizeKHData", _this]) params ["_result", "_returnCode"];

if ([_returnCode] call KH_fnc_parseBoolean) exitWith {
	diag_log (text ([_result, " | EXTENSION = kh_framework | FUNCTION = UnbinarizeKHData | ARGUMENTS = ", _this] joinString ""));
	nil;
}
else {
	true;
};