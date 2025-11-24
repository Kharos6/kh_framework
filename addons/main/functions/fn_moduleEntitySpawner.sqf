isNil {
    params [["_logic", objNull, [objNull]], ["_units", [], [[]]], ["_activated", true, [true]]];
    if (_logic getVariable ["KH_var_moduleActivated", false]) exitWith {};

    if _activated then {
        _logic setVariable ["KH_var_moduleActivated", true];
        
        private _spawnHandler = [
            parseSimpleArray (["[", _logic getVariable ["KH_ModuleEntitySpawnerEntityTypes", ""], "]"] joinString ""),
            [[_logic, [vectorDir _logic, vectorUp _logic]]],
            _logic getVariable ["KH_ModuleEntitySpawnerRadius", [100, 100, 0]],
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

                case "UNIT_GROUP": {
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

                case "AGENT": {
                    ["AGENT", _logic getVariable ["KH_ModuleEntitySpawnerPlacementMode", ""]];
                };

                case "GROUP": {
                    [
                        "GROUP",
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
                        }
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
            _logic getVariable ["KH_ModuleEntitySpawnerCountKilled", true],
            _logic getVariable ["KH_ModuleEntitySpawnerValidatePosition", true],
            parseNumber (_logic getVariable ["KH_ModuleEntitySpawnerMinimumPlayerDistance", "100"]),
            parseNumber (_logic getVariable ["KH_ModuleEntitySpawnerMaximumPlayerDistance", "3000"])
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