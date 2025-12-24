params [["_identifier", "", [""]], ["_overrideAttributes", [], [[]]]];
if (_identifier isEqualTo "") exitWith {};
private _objectPersistency = "khNamespace" readKhData [["objectPersistency_", _identifier] joinString "", createHashMap];

{
    if (_x getVariable ["KH_var_allowPersistency", true]) then {
        private _variableName = vehicleVarName _x;

        if ((_variableName isNotEqualTo "") && !(_x getVariable ["KH_var_generatedVariableName", false])) then {
            private _attributes = _objectPersistency get _variableName;

            if !(isNil "_attributes") then {
                if !(_x getVariable ["KH_var_persistencyTransforms", true]) then {
                    _overrideAttributes insert [-1, [4, 5, 6], true];
                };

                [_x, _attributes, _overrideAttributes, true] call KH_fnc_setObjectAttributes;
            };
        };
    };
} forEach (entities [[], ["Man"], false, true]);

nil;