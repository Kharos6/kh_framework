params [["_identifier", "", [""]], ["_overrideAttributes", [], [[]]]];
if (_identifier isEqualTo "") exitWith {};
private _persistencyNamespace = [_identifier, "persistency"] joinString "_";
private _objectPersistency = _persistencyNamespace readKhData ["objectPersistency", createHashMap];
private _undefinedObjectPersistency = _persistencyNamespace readKhData [["undefinedObjectPersistency", toLowerANSI worldName] joinString "_", []];

{
    private _variableName = vehicleVarName _x;

    if ((_variableName isNotEqualTo "") && !(_x getVariable ["KH_var_generatedVariableName", false])) then {
        private _savedData = _objectPersistency get _variableName;

        if !(isNil "_savedData") then {
            private _currentOverrideAttributes = _overrideAttributes;

            if !(_x getVariable ["KH_var_persistencyTransforms", true]) then {
                _currentOverrideAttributes = +_overrideAttributes;
                _currentOverrideAttributes insert [-1, [4, 5, 6], true];
            };

            [_x, _savedData, _currentOverrideAttributes, true] call KH_fnc_setObjectAttributes;
        };
    };
} forEach ((allMissionObjects "") select {(_x getVariable ["KH_var_allowPersistency", true]) && !(_x isKindOf "Man") && !(_x isKindOf "Logic") && !(_x isKindOf "EmptyDetector") && (((typeOf _x) select [0, 1]) isNotEqualTo "#");});

private _terrainObjectChanges = [];

{
    private _exists = false;

    private _savedData = if ((count _x) isEqualTo 2) then {
        if !(KH_var_edenIds isNil (_x select 0)) then {
            _exists = !(isNull (KH_var_edenIds get (_x select 0)));
        };

        _x select 1;
    }
    else {
        _x;
    };

    if _exists then {
        private _currentEntity = KH_var_edenIds get (_x select 0);
        private _currentOverrideAttributes = _overrideAttributes;

        if !(_currentEntity getVariable ["KH_var_persistencyTransforms", true]) then {
            _currentOverrideAttributes = +_overrideAttributes;
            _currentOverrideAttributes insert [-1, [4, 5, 6], true];
        };

        [_currentEntity, _savedData, _currentOverrideAttributes, true] call KH_fnc_setObjectAttributes;
    }
    else {
        private _object = createVehicle [_savedData select 3, (_savedData select 6) select 4, [], 0, "CAN_COLLIDE"];
        [_object, _savedData, _overrideAttributes, true] call KH_fnc_setObjectAttributes;

        if ((_savedData select 5) isNotEqualTo []) then {
            _terrainObjectChanges pushBack (_savedData select 5);
        };
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