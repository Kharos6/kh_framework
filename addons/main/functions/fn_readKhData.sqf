params [["_file", "", [""]], ["_variable", "", [""]]];
private _i = 0;
private _return = [];

for "_i" from 0 to ((parseNumber (["kh_framework", _this, "SliceKHData", false] call KH_fnc_callExtension)) - 1) do {
	_return pushBack (["kh_framework", [_file, _variable, _i], "ReadKHData", false] call KH_fnc_callExtension);
};

parseSimpleArray (_return joinString "");