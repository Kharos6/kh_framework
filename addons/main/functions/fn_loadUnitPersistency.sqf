params [["_identifier", "", [""]], ["_overrideAttributes", [], [[]]]];
if (_identifier isEqualTo "") exitWith {};
private _unitPersistency = KH_var_khDataNamespace readKhData [["unitPersistency_", _identifier] joinString "", createHashMap];

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

                [_x, _attributes, _currentOverrideAttributes, true] call KH_fnc_setUnitAttributes;
            };
        };
    };
} forEach (KH_var_allMen select {!(isPlayer _x);});

nil;