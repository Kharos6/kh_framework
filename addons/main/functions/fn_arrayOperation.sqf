params [["_formula", [], [[]]]];

if (_formula isEqualTo []) exitWith {
	nil;
};

("kh_framework" callExtension ["ArrayOperation", _formula]) params ["_result", "_returnCode"];

if ([_returnCode] call KH_fnc_parseBoolean) exitWith {
	diag_log (text ([_result, " | EXTENSION = kh_framework | FUNCTION = ArrayOperation | ARGUMENTS = ", _formula] joinString ""));
	nil;
};

if (("[" in _result) && ("]" in _result)) then {
	parseSimpleArray _result;
}
else {
	parseNumber _result;
};