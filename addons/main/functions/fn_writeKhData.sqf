params [["_file", "", [""]], ["_variable", "", [""]], ["_value", nil], ["_type", nil, [""]], ["_overwrite", true, [true]]];

if ((_file isEqualTo "") || (_variable isEqualTo "") || (isNil "_value")) exitWith {
	false;
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
	false;
};

private _result = ("kh_framework" callExtension ["WriteKHData", _this]) select 0;

if ("KH_ERROR: " in _result) then {
	false;
}
else {
	true;
};