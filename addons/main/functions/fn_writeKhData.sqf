params [["_file", "", [""]], ["_variable", "", [""]], ["_type", "", [""]], "_value"];

if (isNil "_value") exitWith {
	[_file, _variable] call KH_fnc_deleteKhDataVariable;
};

["kh_framework", [_file, _variable, [_type, _value] call KH_fnc_serializeValue], "WriteKHData", false] call KH_fnc_callExtension;