[
    KH_var_serverMissionSessionId,
    [
        environmentEnabled,
        timeMultiplier,
        date,
        fogParams,
        wind,
        windDir,
        gusts,
        overcast,
        lightnings,
        rainParams,
        rainbow,
        humidity,
        waves,
        call {
            private _allRelations = [];

            {
                private _currentSide = _x;

                {
                    _allRelations pushBack [_currentSide, _x, _currentSide getFriend _x];    
                } forEach [west, east, resistance, civilian];
            } forEach [west, east, resistance, civilian];

            _allRelations;
        },
        missionNamespace getVariable ["KH_var_persistentVariables", []]
    ]
];