params [["_file", "", [""]], ["_variable", "", [""]], ["_type", "", [""]], "_value", ["_overwrite", true, [true]]];
[_type, _value] call KH_fnc_serializeValue;

if (isNil "_value") exitWith {
	[_file, _variable] call KH_fnc_deleteKhDataVariable;
};

["kh_framework", [_file, _variable, _value, _type, _overwrite], "WriteKHData", false] call KH_fnc_callExtension;