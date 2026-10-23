params [["_identifier", "", [""]], ["_overrideAttributes", [], [[]]]];
if (_identifier isEqualTo "") exitWith {};
private _groupPersistency = ([_identifier, "persistency"] joinString "_") readKhData ["groupPersistency", createHashMap];

{
    private _attributes = _groupPersistency get ([side _x, groupId _x] joinString "_");

    if !(isNil "_attributes") then {
        [_x, _attributes, _overrideAttributes] call KH_fnc_setGroupAttributes;
    };
} forEach allGroups;

nil;