params [["_identifier", "", [""]], ["_groups", allGroups, []]];

if (_identifier isEqualTo "") exitWith {
	createHashMap;
};

private _persistencyId = ["groupPersistency_", _identifier] joinString "";
private _currentEntries = KH_var_khDataNamespace readKhData [_persistencyId, createHashMap];

{
	private _attributes = [_x] call KH_fnc_getGroupAttributes;
	_currentEntries set [[side _x, groupId _x] joinString "_", _attributes];
	_currentEntries set [groupId _x, _attributes];
} forEach _groups;

KH_var_khDataNamespace writeKhData [_persistencyId, _currentEntries];
_currentEntries;