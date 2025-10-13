isNil {
	params [["_logic", objNull, [objNull]], ["_units", [], [[]]], ["_activated", true, [true]]];

    if _activated then {
        params ['_entity', '_position', '_rotation', '_transition', '_heal', '_freefallHeight', '_init', '_name'];
        [
            [_entity, missionNamespace getVariable [_position, objNull], missionNamespace getVariable [_rotation, objNull], _transition, _heal, _freefallHeight, _init, _name],
            {
                params ['_entity', '_position', '_rotation', '_transition', '_heal', '_freefallHeight', '_init', '_name'];
                [
                    _entity,
                    _name,
                    '\a3\data_f_destroyer\data\UI\IGUI\Cfg\holdactions\holdAction_loadVehicle_ca.paa',
                    '\a3\data_f_destroyer\data\UI\IGUI\Cfg\holdactions\holdAction_loadVehicle_ca.paa',
                    '((_this distance _target) < 4)',
                    '((_caller distance _target) < 4)',
                    {},
                    {},
                    {
                        (_this select 3) params ['_entity', '_position', '_rotation', '_transition', '_heal', '_freefallHeight', '_init'];
                        [[player], _position, _rotation, false, _transition, _heal, _freefallHeight, _init] call KH_fnc_teleport;
                    },
                    {},
                    [_entity, _position, _rotation, _transition, _heal, _freefallHeight, _init],
                    1,
                    0,
                    false,
                    false,
                    true
                ] call BIS_fnc_holdActionAdd;
            },
            ['JIP', 'PLAYERS', _entity, false, false, ''],
            'THIS_FRAME'
        ] call KH_fnc_execute;
    };
};

nil;