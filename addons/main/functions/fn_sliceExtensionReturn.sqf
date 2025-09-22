params [["_finalSliceIndex", 0, [0]]];
private _i = 0;
private _return = [];

for "_i" from 0 to _finalSliceIndex do {
	_return pushBack (["kh_framework", _this, "SliceExtensionReturn", false] call KH_fnc_callExtension);
};

_return joinString "";