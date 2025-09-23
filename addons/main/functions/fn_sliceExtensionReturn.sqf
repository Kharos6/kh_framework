params [["_slices", 0, [0]]];
private _i = 0;
private _return = [];

for "_i" from 0 to (_slices - 1) do {
	_return pushBack (["kh_framework", [_i], "SliceExtensionReturn", false] call KH_fnc_callExtension);
};

_return joinString "";