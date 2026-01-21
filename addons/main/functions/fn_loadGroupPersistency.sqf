params [["_identifier", "", [""]], ["_overrideAttributes", [], [[]]]];
if (_identifier isEqualTo "") exitWith {};
private _groupPersistency = KH_var_khDataNamespace readKhData [["groupPersistency_", _identifier] joinString "", createHashMap];

{
    private _groupSideId = [side _x, groupId _x] joinString "_";
    private _attributes = _groupPersistency get _groupSideId;

    if (isNil "_attributes") then {
        _attributes = _groupPersistency get (groupId _x);
        
        if !(isNil "_attributes") then {
            [_x, _attributes, _overrideAttributes] call KH_fnc_setGroupAttributes;
        };
    }
    else {
        [_x, _attributes, _overrideAttributes] call KH_fnc_setGroupAttributes;
    };
} forEach allGroups;

nil;