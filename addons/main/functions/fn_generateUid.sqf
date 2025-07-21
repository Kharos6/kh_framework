("kh_framework" callExtension ["GenerateRandomString", [16, true, true, false]]) params ["_result", "_returnCode"];

if ([_returnCode] call KH_fnc_parseBoolean) exitWith {
	diag_log (text ([_result, " | EXTENSION = kh_framework | FUNCTION = GenerateRandomString | ARGUMENTS = ", [16, true, true, false]] joinString ""));
	nil;
};

[
	selectRandom ["a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z"], 
	_result, 
	clientOwner
] joinString "_";