params [["_method", "", [""]], ["_array", [], [[]]]];
("kh_framework" callExtension ["ArrayOperation", _this]) params ["_result", "_returnCode"];

if ([_returnCode] call KH_fnc_parseBoolean) exitWith {
	diag_log (text ([_result, " | EXTENSION = kh_framework | FUNCTION = ArrayOperation | ARGUMENTS = ", _this] joinString ""));
	nil;
};

if (("[" in _result) && ("]" in _result)) then {
	parseSimpleArray _result;
}
else {
	parseNumber _result;
};