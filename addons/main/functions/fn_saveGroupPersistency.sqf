params [["_identifier", "", [""]], ["_groups", allGroups, []]];

if (_identifier isEqualTo "") exitWith {
	createHashMap;
};

private _persistencyId = ["groupPersistency_", _identifier] joinString "";
private _currentEntries = "khNamespace" readKhData [_persistencyId, createHashMap];

{
	_currentEntries set [groupId _x, [_x] call KH_fnc_getGroupAttributes];
} forEach _groups;

"khNamespace" writeKhData [_persistencyId, _currentEntries];
_currentEntries;