params [["_identifier", "", [""]], ["_overrideAttributes", [], [[]]]];
if (_identifier isEqualTo "") exitWith {};
private _objectPersistency = KH_var_khDataNamespace readKhData [["objectPersistency_", _identifier] joinString "", createHashMap];
private _undefinedId = [toLowerANSI worldName, "_undefinedObjectPersistency_", _identifier] joinString "";
private _undefinedObjectPersistency = KH_var_khDataNamespace readKhData [_undefinedId, []];

{
    if (_x getVariable ["KH_var_allowPersistency", true]) then {
        private _variableName = vehicleVarName _x;

        if ((_variableName isNotEqualTo "") && !(_x getVariable ["KH_var_generatedVariableName", false])) then {
            private _attributes = _objectPersistency get _variableName;

            if !(isNil "_attributes") then {
                private _currentOverrideAttributes = _overrideAttributes;

                if !(_x getVariable ["KH_var_persistencyTransforms", true]) then {
                    _currentOverrideAttributes = +_overrideAttributes;
                    _currentOverrideAttributes insert [-1, [4, 5, 6], true];
                };

                [_x, _attributes, _currentOverrideAttributes, true] call KH_fnc_setObjectAttributes;
            };
        };
    };
} forEach ((allMissionObjects "") select {!(_x isKindOf "Man") && !(_x isKindOf "Logic") && !(_x isKindOf "EmptyDetector") && (((typeOf _x) select [0, 1]) isNotEqualTo "#");});

private _terrainObjectChanges = [];

{
    private _object = createVehicle [_x select 3, (_x select 6) select 4, [], 0, "CAN_COLLIDE"];
    [_object, _x, [], true] call KH_fnc_setObjectAttributes;

    if ((_x select 5) isNotEqualTo []) then {
        _terrainObjectChanges pushBack (_x select 5);
    };
} forEach _undefinedObjectPersistency;

if (_terrainObjectChanges isNotEqualTo []) then {
    execute [
        [_terrainObjectChanges], 
        {
            params ["_terrainObjectChanges"];

            {
                _x params ["_id", "_position"];
                
                (((nearestTerrainObjects [_position, [], 0.01, false, true]) select {
                    private _currentPosition = getPosWorld _x;
                    ([toLowerANSI ((getModelInfo _x) select 0), [(_currentPosition select 0) toFixed 2, (_currentPosition select 1) toFixed 2, 0], (getDir _x) toFixed 2] joinString "_") isEqualTo _id;
                }) param [0, objNull]) hideObject true;
            } forEach _terrainObjectChanges;
        },
        "GLOBAL",
        true,
        true
    ];
};

nil;