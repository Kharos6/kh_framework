params [["_unit", objNull, [objNull, teamMemberNull]], ["_toggle", true, [true]], ["_radius", 100, [0]], ["_enterBuildings", true, [true]], ["_randomDelay", [5, 15, 30], [[]]]];

if (_unit isEqualType teamMemberNull) then {
    _unit = agent _unit;
};

if !(_unit isNil "KH_var_loiterHandler") then {
    [_unit getVariable ["KH_var_loiterHandler", []]] call KH_fnc_removeHandler;
};

if _toggle then {
    doStop _unit;
    private _origin = getPosATL _unit;
    
    _unit setVariable [
        "KH_var_loiterHandler",
        execute [
            [
                _unit, 
                _radius, 
                _enterBuildings,
                _randomDelay,
                _origin, 
                0,
                !(isNull (_unit findNearestEnemy _unit)),
                (nearestTerrainObjects [_origin, ["BUILDING"], _radius, false, true]) + (_origin nearEntities ["Building", _radius]),
                _origin nearRoads _radius
            ],
            {
                params ["_unit", "_radius", "_enterBuildings", "_randomDelay", "_origin", "_currentDelay", "_alerted", "_nearBuildings", "_nearRoads"];

                if ((isNull _unit) || !(alive _unit) || !(local _unit)) exitWith {
                    [_handlerId] call KH_fnc_removeHandler;
                };

                if !(isNull (_unit findNearestEnemy _unit)) exitWith {
                    if !_alerted then {
                        _this set [6, true];
                        _unit setDestination [getPosATL _unit, "DoNotPlan", true];
                        _unit doFollow (leader (group _unit));
                    };
                };

                if _alerted then {
                    _this set [6, false];
                };

                if (_currentDelay >= diag_tickTime) exitWith {};

                if ((getUnitState _unit) isNotEqualTo "WAIT") exitWith {
                    _this set [5, diag_tickTime + random _randomDelay];
                };

                private _position = if (isNull (objectParent _unit)) then { 
                    if _enterBuildings then {
                        if ((_nearBuildings isNotEqualTo []) && ((random 1) > 0.5)) then {
                            private _currentPosition = (selectRandom _nearBuildings) buildingPos -1;

                            if (_currentPosition isNotEqualTo []) then {
                                ASLToATL (AGLToASL (selectRandom _currentPosition));
                            }
                            else {
                                [(_origin select 0) + (random [-_radius, 0, _radius]), (_origin select 1) + (random [-_radius, 0, _radius]), 0];
                            };
                        }
                        else {
                            [(_origin select 0) + (random [-_radius, 0, _radius]), (_origin select 1) + (random [-_radius, 0, _radius]), 0];
                        };
                    }
                    else {
                        [(_origin select 0) + (random [-_radius, 0, _radius]), (_origin select 1) + (random [-_radius, 0, _radius]), 0];
                    };
                }
                else {
                    if (_nearRoads isNotEqualTo []) then {
                        getPosATL (selectRandom _nearRoads);
                    }
                    else {
                        [(_origin select 0) + (random [-_radius, 0, _radius]), (_origin select 1) + (random [-_radius, 0, _radius]), 0];
                    };
                };

                _unit setDestination [_position, "LEADER PLANNED", true];
                _unit doMove _position;
            },
            true,
            1,
            false
        ]
    ];
}
else {
    _unit setDestination [getPosATL _unit, "DoNotPlan", true];
    _unit doFollow (leader (group _unit));
};

nil;