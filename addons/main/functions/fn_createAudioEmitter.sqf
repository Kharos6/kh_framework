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

if (_loop isEqualType []) then {
    _loopInterval = _loop param [1, 1, [0]];
    _loop = _loop param [0, true, [true]];
};

private _effectId = generateUid;
missionNamespace setVariable [_effectId, true, true];

[
    execute [
        [_source, _parameters, _duration, _loop, _loopInterval, _conditionArguments, _conditionFunction, _effectId],
        {
            params ["_source", "_parameters", "_duration", "_loop", "_loopInterval", "_conditionArguments", "_conditionFunction", "_effectId"];
            if (!(missionNamespace getVariable _effectId) || !(_conditionArguments call _conditionFunction)) exitWith {};
            
            private _createEmitter = {
                params ["_source", "_parameters"];

                if (_source isEqualTo []) then {
                    playSound _parameters;
                }
                else {
                    if (_source isEqualType []) then {
                        if !(_source isEqualTypeAll 0) then {
                            private _entity = _source param [0, objNull, [objNull]];
                            private _selection = _source param [1, "", [""]];
                            private _trackingPosition = _source param [2, [0, 0, 0], [[]]];
                            private _startingPosition = ASLToATL (_entity modelToWorldVisualWorld (_entity selectionPosition _selection));
                            private _sourceEmitter = createVehicleLocal ["KH_HelperSquare", _startingPosition, [], 0, "CAN_COLLIDE"];
                            _sourceEmitter attachTo [_entity, _trackingPosition, _selection, true];
                            _sourceEmitter say3D _parameters;
                            _sourceEmitter;
                        }
                        else {
                            private _sourceEmitter = createVehicleLocal ["KH_HelperSquare", ASLToATL _source, [], 0, "CAN_COLLIDE"];
                            _sourceEmitter say3D _parameters;
                            _sourceEmitter;
                        };
                    }
                    else {
                        private _sourceEmitter = createVehicleLocal ["KH_HelperSquare", getPosATLVisual _source, [], 0, "CAN_COLLIDE"];
                        _sourceEmitter attachTo [_source, [0, 0, 0], "", true];
                        _sourceEmitter say3D _parameters;
                        _sourceEmitter;
                    };
                };
            };

            private _emitter = [_source, _parameters] call _createEmitter;

            execute [
                [
                    _conditionArguments, 
                    _conditionFunction, 
                    _loop, 
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
                    0,
                    _source,
                    _parameters,
                    _createEmitter
                ],
                {
                    params ["_conditionArguments", "_conditionFunction", "_loop", "_loopInterval", "_effectId", "_emitter", "_timeout", "_loopTimeout", "_loopCount", "_source", "_parameters", "_createEmitter"];

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

                    if _loop then {
                        if ((diag_tickTime >= _loopTimeout) || (isNull _emitter)) then {
                            deleteVehicle _emitter;
                            _emitter = [_source, _parameters] call _createEmitter;
                            _this set [5, _emitter];
                            _this set [7, diag_tickTime + _loopInterval];
                            _this set [8, _loopCount + 1];
                        };
                    };
                },
                true,
                0,
                false
            ];

            nil;
        },
        _target,
        true,
        _jip
    ],
    [missionNamespace, _effectId, true]
];