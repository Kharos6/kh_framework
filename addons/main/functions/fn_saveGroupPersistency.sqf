params [["_identifier", "", [""]], ["_groups", allGroups, []]];

if (_identifier isEqualTo "") exitWith {
	createHashMap;
};

private _persistencyNamespace = [_identifier, "persistency"] joinString "_";
private _currentEntries = _persistencyNamespace readKhData ["groupPersistency", createHashMap];

{
	_currentEntries set [[side _x, groupId _x] joinString "_", [_x] call KH_fnc_getGroupAttributes];
} forEach _groups;

_persistencyNamespace writeKhData ["groupPersistency", _currentEntries];
_currentEntries;