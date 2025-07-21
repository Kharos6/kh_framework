params [["_hashType", "", [""]], ["_string", "", [""]]];

if ((_hashType isEqualTo "") || (_string isEqualTo "")) exitWith {
	nil;
};

("kh_framework" callExtension ["CryptoOperation", _this]) params ["_result", "_returnCode"];

if ([_returnCode] call KH_fnc_parseBoolean) exitWith {
	diag_log (text ([_result, " | EXTENSION = kh_framework | FUNCTION = CryptoOperation | ARGUMENTS = ", _this] joinString ""));
	nil;
}
else {
	_result;
};