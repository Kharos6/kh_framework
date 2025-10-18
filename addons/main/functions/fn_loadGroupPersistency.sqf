params [["_identifier", "", [""]], ["_overrideAttributes", [], [[]]]];
if (_identifier isEqualTo "") exitWith {};
private _groupPersistency = "khNamespace" readKhData [["groupPersistency_", _identifier] joinString "", createHashMap];

{
    private _attributes = _groupPersistency get (groupId _x);

    if !(isNil "_attributes") then {
        if (([side _x, groupId _x] joinString "_") isEqualTo ([_attributes select 3, (_attributes select 4) select 1] joinString "_")) then { 
            [_x, _attributes, _overrideAttributes] call KH_fnc_setGroupAttributes;
        };
    };
} forEach allGroups;

nil;