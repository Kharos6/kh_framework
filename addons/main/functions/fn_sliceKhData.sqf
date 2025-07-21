params [["_file", "", [""]], ["_variable", "", [""]]];

if ((_file isEqualTo "") || (_variable isEqualTo "")) exitWith {
	nil;
};

("kh_framework" callExtension ["SliceKHData", _this]) params ["_result", "_returnCode"];

if ([_returnCode] call KH_fnc_parseBoolean) exitWith {
	diag_log (text ([_result, " | EXTENSION = kh_framework | FUNCTION = SliceKHData | ARGUMENTS = ", _this] joinString ""));
	nil;
};

parseNumber _result;