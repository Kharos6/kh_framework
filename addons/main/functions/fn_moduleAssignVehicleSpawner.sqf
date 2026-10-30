isNil {
    KH_var_postInitExecutions pushBack [
        _this,
        {
            params [["_logic", objNull, [objNull]], ["_units", [], [[]]], ["_activated", true, [true]]];

            if _activated then {
                {
                    [
                        _x,
                        _logic,
                        _logic,
                        parseSimpleArray (["[", _logic getVariable ["KH_ModuleAssignVehicleSpawnerVehicleTypes", ""], "]"] joinString "")
                    ] call KH_fnc_assignVehicleSpawner;
                } forEach _units;
            };
        }
    ];
};

nil;