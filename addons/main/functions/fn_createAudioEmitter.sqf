params [
    ["_source", [0, 0, 0], [[], objNull]],
    ["_parameters", "", ["", []]],
    ["_duration", 0, [0]],
    ["_loop", false, [true, []]],
    ["_conditionArguments", []],
    ["_conditionFunction", {true;}, [{}]], 
    ["_target", true, [true, 0, "", [], {}, objNull, teamMemberNull, grpNull, sideUnknown, locationNull]],
    ["_jip", false, [true, []]]
];

if (_duration isEqualTo 0) then {
    _duration = 999999;
};

private _loopInterval = 1;
private _looped = false;

if (_loop isEqualType []) then {
    _looped = _loop param [0, true, [true]];
    _loopInterval = _loop param [1, 1, [0]];
};

private _effectId = generateUid;
missionNamespace setVariable [_effectId, true, true];

[
    [
        [_source, _parameters, _duration, _looped, _loopInterval, _conditionArguments, _conditionFunction, _effectId],
        {
            params ["_source", "_parameters", "_duration", "_looped", "_loopInterval", "_conditionArguments", "_conditionFunction", "_effectId"];
            if (!(missionNamespace getVariable _effectId) || !(_conditionArguments call _conditionFunction)) exitWith {};
            
            private _emitter = if (_source isEqualTo []) then {
                playSound _parameters;
            }
            else {
                createVehicle ["KH_HelperSquare", [0, 0, 0], [], 0, "CAN_COLLIDE"];
                
                if (_source isEqualType []) then {
                    if !(_source isEqualTypeAll 0) then {
                        private _entity = _source param [0, objNull, [objNull]];
                        private _selection = _source param [1, "", [""]];
                        private _trackingPosition = _source param [2, [], [[]]];
                        private _startingPosition = ASLToATL (_entity modelToWorldVisualWorld (_entity selectionPosition _selection));
                        private _sourceEmitter = createVehicleLocal ["KH_HelperSquare", _startingPosition, [], 0, "CAN_COLLIDE"];
                        _sourceEmitter attachTo [_entity, _trackingPosition, _selection, true];
                        _sourceEmitter;
                    }
                    else {
                        private _sourceEmitter = createVehicleLocal ["KH_HelperSquare", ASLToATL _source, [], 0, "CAN_COLLIDE"];
                        _sourceEmitter say3D _parameters;
                        _sourceEmitter;
                    };
                }
                else {
                    private _sourceEmitter = createVehicleLocal ["KH_HelperSquare", ASLToATL _source, [], 0, "CAN_COLLIDE"];
                    _sourceEmitter attachTo [_source, [0, 0, 0], "", true];
                    _sourceEmitter say3D _parameters;
                    _sourceEmitter;
                };
            };

            [
                [
                    _conditionArguments, 
                    _conditionFunction, 
                    _looped, 
                    _loopInterval, 
                    _effectId, 
                    _emitter, 
                    if (_duration < 0) then {
                        _duration;
                    }
                    else {
                        diag_tickTime + _duration;
                    }, 
                    diag_tickTime + _loopInterval,
                    0
                ],
                {
                    params ["_conditionArguments", "_conditionFunction", "_looped", "_loopInterval", "_effectId", "_emitter", "_timeout", "_loopTimeout", "_loopCount"];

                    private _invalid = if (_timeout < 0) then {
                        _loopCount >= (abs _timeout);
                    }
                    else {
                        diag_tickTime > _timeout;
                    };

                    if (!(missionNamespace getVariable _effectId) || !(_conditionArguments call _conditionFunction) || _invalid) exitWith {
                        deleteVehicle _emitter;
                        [_handlerId] call KH_fnc_removeHandler;
                    };

                    if _looped then {
                        if ((diag_tickTime >= _loopTimeout) || (isNull _emitter)) then {
                            deleteVehicle _emitter;

                            _emitter = if (_source isEqualTo []) then {
                                playSound _parameters;
                            }
                            else {
                                createVehicle ["KH_HelperSquare", [0, 0, 0], [], 0, "CAN_COLLIDE"];
                                
                                if (_source isEqualType []) then {
                                    if !(_source isEqualTypeAll 0) then {
                                        private _entity = _source param [0, objNull, [objNull]];
                                        private _selection = _source param [1, "", [""]];
                                        private _trackingPosition = _source param [2, [], [[]]];
                                        private _startingPosition = ASLToATL (_entity modelToWorldVisualWorld (_entity selectionPosition _selection));
                                        private _sourceEmitter = createVehicleLocal ["KH_HelperSquare", _startingPosition, [], 0, "CAN_COLLIDE"];
                                        _sourceEmitter attachTo [_entity, _trackingPosition, _selection, true];
                                        _sourceEmitter;
                                    }
                                    else {
                                        private _sourceEmitter = createVehicleLocal ["KH_HelperSquare", ASLToATL _source, [], 0, "CAN_COLLIDE"];
                                        _sourceEmitter say3D _parameters;
                                        _sourceEmitter;
                                    };
                                }
                                else {
                                    private _sourceEmitter = createVehicleLocal ["KH_HelperSquare", ASLToATL _source, [], 0, "CAN_COLLIDE"];
                                    _sourceEmitter attachTo [_source, [0, 0, 0], "", true];
                                    _sourceEmitter say3D _parameters;
                                    _sourceEmitter;
                                };
                            };
                            
                            _this set [5, _emitter];
                            _this set [7, diag_tickTime + _loopInterval];
                            _this set [8, _loopCount + 1];
                        };
                    };
                },
                true,
                0,
                false
            ] call KH_fnc_execute;

            nil;
        },
        _target,
        true,
        _jip
    ] call KH_fnc_execute,
    [missionNamespace, _effectId, true]
];