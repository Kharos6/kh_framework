params [["_file", "", [""]], ["_variable", "", [""]]];

if ((_file isEqualTo "") || (_variable isEqualTo "")) exitWith {
	false;
};

private _result = ("kh_framework" callExtension ["SliceData", _this]) select 0;

if ("KH_ERROR: " in _result) then {
	false;
}
else {
	parseNumber _result;
};