params [["_identifier", "", [""]], ["_overrideAttributes", [], [[]]]];
if (_identifier isEqualTo "") exitWith {};
private _unitPersistency = KH_var_khDataNamespace readKhData [["unitPersistency_", _identifier] joinString "", createHashMap];
private _undefinedId = [toLowerANSI worldName, "_undefinedUnitPersistency_", _identifier] joinString "";
private _undefinedUnitPersistency = KH_var_khDataNamespace readKhData [_undefinedId, []];

{  
    if (_x getVariable ["KH_var_allowPersistency", true]) then {
        private _variableName = vehicleVarName _x;

        if ((_variableName isNotEqualTo "") && !(_x getVariable ["KH_var_generatedVariableName", false])) then {
            private _attributes = _unitPersistency get _variableName;

            if !(isNil "_attributes") then {
                private _currentOverrideAttributes = _overrideAttributes;

                if !(_x getVariable ["KH_var_persistencyTransforms", true]) then {
                    _currentOverrideAttributes = +_overrideAttributes;
                    _currentOverrideAttributes insert [-1, [26, 27, 28], true];
                };

                private _group = ["GROUP", [_attributes select 39, _attributes select 40] joinString "_"] call KH_fnc_getEntityByIdentifier;

                if (isNull _group) then {
                    _group = createGroup [_attributes select 39, true];
                    _group setGroupIdGlobal [_attributes select 40];
                };

                [_x, _attributes, _currentOverrideAttributes, true] call KH_fnc_setUnitAttributes;
            };
        };
    };
} forEach (KH_var_allMen select {!(isPlayer _x);});

{
    private _attributes = _x select 5;
    private _group = ["GROUP", [_attributes select 39, _attributes select 40] joinString "_"] call KH_fnc_getEntityByIdentifier;

    if (isNull _group) then {
        _group = createGroup [_attributes select 39, true];
        _group setGroupIdGlobal [_attributes select 40];
    };

    [_group createUnit [_x select 3, _attributes select 26, [], 0, "CAN_COLLIDE"], _x, [], true] call KH_fnc_setUnitAttributes;
} forEach _undefinedUnitPersistency;

nil;