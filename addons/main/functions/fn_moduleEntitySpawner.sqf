KH_var_postInitExecutions pushBack [
    _this,
    {
        params [["_logic", objNull, [objNull]], ["_units", [], [[]]], ["_activated", true, [true]]];

        if _activated then {
            private _spawnHandler = [
                parseSimpleArray (["[", _logic getVariable ["KH_ModuleEntitySpawnerEntityTypes", ""], "]"] joinString ""),
                [[_logic, [vectorDir _logic, vectorUp _logic]]],
                parseSimpleArray (["[", _logic getVariable ["KH_ModuleEntitySpawnerRadius", "100, 100, 0"], "]"] joinString ""),
                parseNumber (_logic getVariable ["KH_ModuleEntitySpawnerAmount", "1"]),
                parseNumber (_logic getVariable ["KH_ModuleEntitySpawnerMaximum", "1"]),
                compile (_logic getVariable ["KH_ModuleEntitySpawnerCondition", "true"]),
                compile (_logic getVariable ["KH_ModuleEntitySpawnerInit", ""]),
                switch (_logic getVariable ["KH_ModuleEntitySpawnerType", ""]) do {
                    case "UNIT": {
                        [
                            "UNIT",
                            _logic getVariable ["KH_ModuleEntitySpawnerPlacementMode", ""],
                            switch (_logic getVariable ["KH_ModuleEntitySpawnerSide", ""]) do {
                                case "BLUFOR": {
                                    west;
                                };

                                case "OPFOR": {
                                    east;
                                };

                                case "GREENFOR": {
                                    resistance;
                                };

                                case "CIVILIAN": {
                                    civilian;
                                };
                            },
                            false
                        ];
                    };

                    case "AGENT": {
                        ["AGENT", _logic getVariable ["KH_ModuleEntitySpawnerPlacementMode", ""]];
                    };

                    case "GROUP": {
                        [
                            "UNIT",
                            _logic getVariable ["KH_ModuleEntitySpawnerPlacementMode", ""],
                            switch (_logic getVariable ["KH_ModuleEntitySpawnerSide", ""]) do {
                                case "BLUFOR": {
                                    west;
                                };

                                case "OPFOR": {
                                    east;
                                };

                                case "GREENFOR": {
                                    resistance;
                                };

                                case "CIVILIAN": {
                                    civilian;
                                };
                            },
                            true
                        ];
                    };

                    case "OBJECT": {
                        ["OBJECT", _logic getVariable ["KH_ModuleEntitySpawnerPlacementMode", ""], false];
                    };

                    case "SIMPLE_OBJECT": {
                        ["SIMPLE_OBJECT", false];
                    };

                    case "VEHICLE": {
                        ["VEHICLE", _logic getVariable ["KH_ModuleEntitySpawnerPlacementMode", ""]];
                    };
                },
                parseNumber (_logic getVariable ["KH_ModuleEntitySpawnerInterval", "1"]),
                _logic getVariable ["KH_ModuleEntitySpawnerValidatePosition", true],
                parseNumber (_logic getVariable ["KH_ModuleEntitySpawnerMinimumPlayerDistance", "0"])
            ] call KH_fnc_entitySpawner;

            [
                ["ENTITY", _logic, "REMOTE"],
                "Deleted",
                [_spawnHandler],
                {
                    _args params ["_spawnHandler"];
                    [_spawnHandler] call KH_fnc_removeHandler;
                }
            ] call KH_fnc_addEventHandler;
        };
    }
];

nil;