params [["_function", "", [""]], ["_arguments", [], [[]]]];

if ((_function isEqualTo "") || (_arguments isEqualTo [])) exitWith {
	false;
};

if !((_arguments select 0) isEqualType "") exitWith {
	false;
};

private _result = ("kh_framework" callExtension ["VectorOperation", _this]) select 0;

if ("KH_ERROR: " in _result) exitWith {
	false;
};

if (("[" in _result) && ("]" in _result)) then {
	parseSimpleArray _result;
}
else {
	parseNumber _result;
};