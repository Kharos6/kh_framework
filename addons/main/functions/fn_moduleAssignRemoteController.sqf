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
                            "Access Remote Controller",
                            "\a3\ui_f\data\IGUI\Cfg\holdactions\holdAction_hack_ca.paa",
                            "\a3\ui_f\data\IGUI\Cfg\holdactions\holdAction_hack_ca.paa",
                            "((_this distance _target) < 6)",
                            "((_caller distance _target) < 6)",
                            {},
                            {},
                            {
                                isNil {
                                    [(units (side (group player))) select {!(isPlayer _x);}] call KH_fnc_openRemoteController;
                                };
                            },
                            {},
                            [],
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