params [["_object", objNull, [objNull]], ["_position", [0, 0, 0], [[], objNull]], ["_rotation", [[0, 1, 0], [0, 0, 1]], [[], objNull]], ["_types", [], [[]]]];

execute [
    [
        _object,
        "Access Vehicle Depot",
        "\a3\data_f_destroyer\data\UI\IGUI\Cfg\holdactions\holdAction_loadVehicle_ca.paa",
        "\a3\data_f_destroyer\data\UI\IGUI\Cfg\holdactions\holdAction_loadVehicle_ca.paa",
        "((_this distance _target) < 6)",
        "((_caller distance _target) < 6)",
        {},
        {},
        {
            isNil {
                (_this select 3) call KH_fnc_openVehicleSpawner;
            };
        },
        {},
        [
            _types,
            _position,
            _rotation
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
];