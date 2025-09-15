[
    KH_var_missionSessionId,
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
                    _allRelations pushBack [["SIDE", _currentSide] call KH_fnc_serializeValue, ["SIDE", _x] call KH_fnc_serializeValue, _currentSide getFriend _x];    
                } forEach [west, east, resistance, civilian];
            } forEach [west, east, resistance, civilian];

            _allRelations;
        }
        call {
            private _allVariables = [];

			{
				_x params [["_name", "", [""]], "_value", ["_public", false, [true]]];
				_allVariables pushBack [_name, ["", _value] call KH_fnc_serializeValue, _public];
			} forEach (missionNamespace getVariable ["KH_var_persistentVariables", []]);

            _allVariables;
        }
    ]
];