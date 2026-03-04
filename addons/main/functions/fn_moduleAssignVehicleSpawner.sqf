isNil {
    KH_var_postInitExecutions pushBack [
        _this,
        {
            params [["_logic", objNull, [objNull]], ["_units", [], [[]]], ["_activated", true, [true]]];

            if _activated then {
                {
                    [
                        [
                            _x,
                            "Access Vehicle Depot",
                            "\a3\data_f_destroyer\data\UI\IGUI\Cfg\holdactions\holdAction_loadVehicle_ca.paa",
                            "\a3\data_f_destroyer\data\UI\IGUI\Cfg\holdactions\holdAction_loadVehicle_ca.paa",
                            "((_this distance _target) < 6)",
                            "((_caller distance _target) < 6)",
                            {},
                            {},
                            {
                                isNil {
                                    (_this select 3) call KH_fnc_vehicleSpawner;
                                };
                            },
                            {},
                            [
                                parseSimpleArray (["[", _logic getVariable ["KH_ModuleAssignVehicleSpawnerVehicleTypes", ""], "]"] joinString ""),
                                _logic,
                                _logic
                            ],
                            1,
                            0,
                            false,
                            false,
                            true
                        ],
                        "BIS_fnc_holdActionAdd", 
                        "PLAYERS",
                        true,
                        ["JIP", _x, false, ""]
                    ] call KH_fnc_execute;
                } forEach _units;
            };
        }
    ];
};

nil;