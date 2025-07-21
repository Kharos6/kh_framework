params [["_file", "", [""]], ["_variable", "", [""]], ["_value", nil], ["_type", nil, [""]], ["_overwrite", true, [true]]];

if ((_file isEqualTo "") || (_variable isEqualTo "") || (isNil "_value")) exitWith {
	nil;
};

if (isNil "_type") then {
	_type = typeName _value;
};

if (
	((_type isNotEqualTo "ARRAY") && 
	(_type isNotEqualTo "STRING") && 
	(_type isNotEqualTo "SCALAR") && 
	(_type isNotEqualTo "HASHMAP") && 
	(_type isNotEqualTo "BOOL") && 
	(_type isNotEqualTo "CODE") && 
	(_type isNotEqualTo "TEXT")) ||
	((_type isEqualTo "CODE") && ((([_value] joinString "") select [0, 1]) isEqualTo "{"))
   ) exitWith {
	diag_log (text (["KH_ERROR: BAD DATA TYPE", " | EXTENSION = kh_framework | FUNCTION = WriteKHData | ARGUMENTS = ", _this] joinString ""));
	nil;
};

("kh_framework" callExtension ["WriteKHData", _this]) params ["_result", "_returnCode"];

if ([_returnCode] call KH_fnc_parseBoolean) then {
	diag_log (text ([_result, " | EXTENSION = kh_framework | FUNCTION = WriteKHData | ARGUMENTS = ", _this] joinString ""));
	nil;
}
else {
	true;
};