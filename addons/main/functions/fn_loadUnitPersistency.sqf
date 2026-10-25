params [["_identifier", "", [""]], ["_overrideAttributes", [], [[]]]];
if (_identifier isEqualTo "") exitWith {};
private _persistencyNamespace = [_identifier, "persistency"] joinString "_";
private _unitPersistency = _persistencyNamespace readKhData ["unitPersistency", createHashMap];
private _undefinedUnitPersistency = _persistencyNamespace readKhData [["undefinedUnitPersistency", toLowerANSI worldName] joinString "_", []];

{  
    private _variableName = vehicleVarName _x;

    if ((_variableName isNotEqualTo "") && !(_x getVariable ["KH_var_generatedVariableName", false])) then {
        private _savedData = _unitPersistency get _variableName;

        if !(isNil "_savedData") then {
            private _attributes = _savedData select 5;
            private _currentOverrideAttributes = _overrideAttributes;

            if !(_x getVariable ["KH_var_persistencyTransforms", true]) then {
                _currentOverrideAttributes = +_overrideAttributes;
                _currentOverrideAttributes insert [-1, [26, 27, 28], true];
            };

            if !(_savedData select 4) then {
                private _group = ["GROUP", [_attributes select 39, _attributes select 40] joinString "_"] call KH_fnc_getEntityByIdentifier;

                if (isNull _group) then {
                    _group = createGroup [_attributes select 39, true];
                    _group setGroupIdGlobal [_attributes select 40];
                };
            };

            [_x, _savedData, _currentOverrideAttributes, true] call KH_fnc_setUnitAttributes;
        };
    };
} forEach (KH_var_allMen select {(_x getVariable ["KH_var_allowPersistency", true]) && !(isPlayer _x);});

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

    private _attributes = _savedData select 5;

    if _exists then {
        private _currentEntity = KH_var_edenIds get (_x select 0);
        private _currentOverrideAttributes = _overrideAttributes;

        if !(_currentEntity getVariable ["KH_var_persistencyTransforms", true]) then {
            _currentOverrideAttributes = +_overrideAttributes;
            _currentOverrideAttributes insert [-1, [26, 27, 28], true];
        };

        [_currentEntity, _savedData, _currentOverrideAttributes, true] call KH_fnc_setUnitAttributes;
    }
    else {
        private _group = ["GROUP", [_attributes select 39, _attributes select 40] joinString "_"] call KH_fnc_getEntityByIdentifier;

        if !(_savedData select 4) then {
            if (isNull _group) then {
                _group = createGroup [_attributes select 39, true];
                _group setGroupIdGlobal [_attributes select 40];
            };

            [_group createUnit [_savedData select 3, _attributes select 26, [], 0, "CAN_COLLIDE"], _savedData, _overrideAttributes, true] call KH_fnc_setUnitAttributes;
        }
        else {
            [createAgent [_savedData select 3, _attributes select 26, [], 0, "CAN_COLLIDE"], _savedData, _overrideAttributes, true] call KH_fnc_setUnitAttributes;
        };
    };
} forEach _undefinedUnitPersistency;

nil;