isNil {
	params [["_logic", objNull, [objNull]], ["_units", [], [[]]], ["_activated", true, [true]]];

    if _activated then {
        {
            [
                [
                    getPosATL _logic,
                    [vectorDir _logic, vectorUp _logic],
                    parseNumber (_logic getVariable ["KH_ModuleTeleporterTransition", "1"]), 
                    parseNumber (_logic getVariable ["KH_ModuleTeleporterFreefallHeight", "-1"]), 
                    compile (_logic getVariable ["KH_ModuleTeleporterInit", ""]), 
                    _logic getVariable ["KH_ModuleTeleporterName", ""],
                    _x
                ],
                {
                    params ["_position", "_rotation", "_transition", "_freefallHeight", "_init", "_name", "_teleporter"];
                    
                    [
                        _teleporter,
                        _name,
                        '\a3\data_f_destroyer\data\UI\IGUI\Cfg\holdactions\holdAction_loadVehicle_ca.paa',
                        '\a3\data_f_destroyer\data\UI\IGUI\Cfg\holdactions\holdAction_loadVehicle_ca.paa',
                        '((_this distance _target) < 4)',
                        '((_caller distance _target) < 4)',
                        {},
                        {},
                        {
                            (_this select 3) params ["_position", "_rotation", "_transition", "_freefallHeight", "_init"];
                            [_caller, _position, _rotation, false, _transition, _freefallHeight, _init] call KH_fnc_teleport;
                        },
                        {},
                        [_position, _rotation, _transition, _freefallHeight, _init],
                        1,
                        0,
                        false,
                        false,
                        true
                    ] call BIS_fnc_holdActionAdd;
                },
                "PLAYERS",
                true,
                ["JIP", _x, true, ""]
            ] call KH_fnc_execute;
        } forEach _units;
    };
};

nil;