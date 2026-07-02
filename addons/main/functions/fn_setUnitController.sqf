params [
    ["_unit", objNull, [objNull]],
    ["_turningSpeed", 120, [0]], 
    ["_stopTurnThreshold", 22.5, [0]], 
    ["_acceptedDeviationAngle", 0.5, [0]],
    ["_acceptedDestinationDistance", 1, [0]],
    ["_forcedWalkDistanceThreshold", 0, [0]],
    ["_forcedJogDistanceThreshold", 0, [0]],
    ["_condition", {true;}, [{}]],
    ["_animationCondition", {true;}, [{}]]
];

private _id = generateUid;
missionNamespace setVariable [_id, true, true];
_unit setVariable ["KH_var_unitControllerId", _id, true];

[
    [
        [
            _unit, 
            _id,
            _turningSpeed, 
            _stopTurnThreshold, 
            _acceptedDeviationAngle, 
            _acceptedDestinationDistance,
            _forcedWalkDistanceThreshold,
            _forcedJogDistanceThreshold,
            _condition,
            _animationCondition
        ],
        {
            params [
                "_unit", 
                "_id", 
                "_turningSpeed", 
                "_stopTurnThreshold", 
                "_acceptedDeviationAngle", 
                "_acceptedDestinationDistance", 
                "_forcedWalkDistanceThreshold",
                "_forcedJogDistanceThreshold",
                "_condition",
                "_animationCondition"
            ];

            _unit setVariable ["KH_var_preUnitControllerAnimationFeature", _unit checkAIFeature "ANIM"];
            _unit enableAIFeature ["ANIM", false];

            _unit setVariable [
                "KH_var_unitControllerHandler",
                [
                    [
                        _unit, 
                        _id, 
                        [], 
                        "", 
                        "",
                        false,
                        true,
                        (_unit getVariable ["KH_var_movesActionsConfig", configNull]), 
                        (_unit getVariable ["KH_var_movesStatesConfig", configNull]),
                        _turningSpeed, 
                        _stopTurnThreshold,
                        _acceptedDeviationAngle, 
                        _acceptedDestinationDistance,
                        _forcedWalkDistanceThreshold,
                        _forcedJogDistanceThreshold,
                        _condition,
                        _animationCondition
                    ],
                    {
                        params [
                            "_unit", 
                            "_id", 
                            "_currentPath", 
                            "_currentPathId", 
                            "_animation",
                            "_moveGracePeriod",
                            "_validateCollision",
                            "_actions", 
                            "_states", 
                            "_turningSpeed", 
                            "_stopTurnThreshold", 
                            "_acceptedDeviationAngle", 
                            "_acceptedDestinationDistance", 
                            "_forcedWalkDistanceThreshold",
                            "_forcedJogDistanceThreshold",
                            "_condition",
                            "_animationCondition"
                        ];

                        if (((_unit getVariable ["KH_var_unitControllerId", ""]) isNotEqualTo _id) || !([_unit] call _condition) || !(missionNamespace getVariable _id)) exitWith {
                            _unit enableAIFeature ["ANIM", _unit getVariable ["KH_var_preUnitControllerAnimationFeature", true]];
                            [_handlerId] call KH_fnc_removeHandler;
                        };

                        if ((isNull (remoteControlled _unit)) && (isNull (objectParent _unit)) && (isTouchingGround _unit)) then {
                            if (_unit checkAIFeature "ANIM") then {
                                _unit enableAIFeature ["ANIM", false];
                            };

                            _moveGracePeriod = if (((_unit getUnitMovesInfo 0) isEqualTo 1) && ((getNumber (_states >> (animationState _unit) >> "looped")) isNotEqualTo 1)) then {
                                if _moveGracePeriod then {
                                    _unit playActionNow "Stop";
                                    false;
                                }
                                else {
                                    true;  
                                };
                            }
                            else {
                                false;
                            };

                            _this set [5, _moveGracePeriod];

                            if (_currentPathId isNotEqualTo (_unit getVariable ["KH_var_calculatedPathId", ""])) then {
                                _currentPath = +(_unit getVariable ["KH_var_calculatedPath", []]);
                                _this set [2, _currentPath];

                                if ((count _currentPath) isNotEqualTo 1) then {
                                    _currentPath deleteAt 0;
                                };

                                _currentPathId = _unit getVariable ["KH_var_calculatedPathId", ""];
                                _this set [3, _currentPathId];
                                _this set [6, false];
                            };

                            if (_currentPath isNotEqualTo []) then {
                                private _currentStep = _currentPath param [0, [0, 0, 0]];

                                private _destinationObject = if (_currentStep isEqualType objNull) then {
                                    _currentStep = getPosASLVisual _currentStep;
                                    alive _destinationObject;
                                }
                                else {
                                    false;
                                };

                                private _relativeDirection = _unit getRelDir _currentStep;
                                private _correctedDirection = abs (((_relativeDirection + 180) % 360) - 180);
                                private _direction = _relativeDirection < 180;
                                private _turnAmount = ((_turningSpeed * _totalDelta) min _correctedDirection);
                                private _heading = ((getDir _unit) + ([-_turnAmount, _turnAmount] select _direction)) % 360;
                                private _headingCorrect = _correctedDirection <= _acceptedDeviationAngle;

                                if !_headingCorrect then {
                                    _unit setVectorDir [sin _heading, cos _heading, 0];
                                };

                                private _stepDistance = (getPosASLVisual _unit) vectorDistance _currentStep;

                                if (_stepDistance <= _acceptedDestinationDistance) then {
                                    if (_validateCollision && ((vectorMagnitude (_unit getUnitMovesInfo 4)) isNotEqualTo 0)) then {
                                        _unit setVelocity [0, 0, 0];
                                        _this set [6, false];
                                    };

                                    if _headingCorrect then {
                                        private _refreshedPathId = _unit getVariable ["KH_var_calculatedPathId", ""];

                                        private _pathRefreshed = if (_currentPathId isNotEqualTo _refreshedPathId) then {
                                            _currentPath = +(_unit getVariable ["KH_var_calculatedPath", []]);
                                            _this set [2, _currentPath];

                                            if ((count _currentPath) isNotEqualTo 1) then {
                                                _currentPath deleteAt 0;
                                            };

                                            _currentPathId = _refreshedPathId;
                                            _this set [3, _currentPathId];
                                            true;
                                        }
                                        else {
                                            false;
                                        };

                                        if (_pathRefreshed || !_destinationObject) then {
                                            _currentPath deleteAt 0;

                                            if (_currentPath isEqualTo []) then {
                                                private _currentAnimation = animationState _unit;
                                                _animation = toLowerANSI (getText (_actions >> (getText (_states >> _currentAnimation >> "actions")) >> "Stop"));
                                                _this set [4, _animation];

                                                if ((_currentAnimation isNotEqualTo _animation) && !(_animation in (flatten (getAnimationsQueue _unit))) && ([_unit, 0] call _animationCondition)) then {
                                                    _unit playActionNow "Stop";
                                                };
                                            };

                                            _this set [6, false];
                                        }
                                        else {
                                            private _currentAnimation = animationState _unit;
                                            _animation = toLowerANSI (getText (_actions >> (getText (_states >> _currentAnimation >> "actions")) >> "Stop"));
                                            _this set [4, _animation];

                                            if ((_currentAnimation isNotEqualTo _animation) && !(_animation in (flatten (getAnimationsQueue _unit))) && ([_unit, 0] call _animationCondition)) then {
                                                _unit playActionNow "Stop";
                                            };
                                        };
                                    }
                                    else {
                                        private _currentAnimation = animationState _unit;
                                        _animation = toLowerANSI (getText (_actions >> (getText (_states >> _currentAnimation >> "actions")) >> "Stop"));
                                        _this set [4, _animation];

                                        if ((_currentAnimation isNotEqualTo _animation) && !(_animation in (flatten (getAnimationsQueue _unit))) && ([_unit, 0] call _animationCondition)) then {
                                            _unit playActionNow "Stop";
                                        };
                                    };
                                }
                                else {
                                    if (_correctedDirection <= _stopTurnThreshold) then {
                                        private _currentAction = if (_stepDistance <= _forcedWalkDistanceThreshold) then {
                                            "WALKF";
                                        }
                                        else {
                                            if (_stepDistance <= _forcedJogDistanceThreshold) then {
                                                "TACTF";
                                            }
                                            else {
                                                [[_unit, false] call KH_fnc_getUnitMoveType, "F"] joinString "";
                                            };
                                        };

                                        if ((_stepDistance <= 5) && ("FAST" in _currentAction)) then {
                                            _currentAction = "TACTF";
                                        };

                                        private _currentAnimation = animationState _unit;
                                        _animation = toLowerANSI (getText (_actions >> (getText (_states >> _currentAnimation >> "actions")) >> _currentAction));
                                        _this set [4, _animation];

                                        if ((_currentAnimation isNotEqualTo _animation) && !(_animation in (flatten (getAnimationsQueue _unit))) && ([_unit, 0] call _animationCondition)) then {
                                            _unit playActionNow _currentAction;
                                        };
                                    }
                                    else {
                                        private _currentAction = ["TurnL", "TurnR"] select _direction;
                                        private _currentAnimation = animationState _unit;
                                        _animation = toLowerANSI (getText (_actions >> (getText (_states >> _currentAnimation >> "actions")) >> _currentAction));
                                        _this set [4, _animation];

                                        if ((_currentAnimation isNotEqualTo _animation) && !(_animation in (flatten (getAnimationsQueue _unit))) && ([_unit, 0] call _animationCondition)) then {
                                            _unit playActionNow _currentAction;
                                        };
                                    };

                                    _this set [6, false];
                                };
                            }
                            else {
                                if ([_unit, 0] call _animationCondition) then {
                                    private _currentAnimation = animationState _unit;
                                    _animation = toLowerANSI (getText (_actions >> (getText (_states >> _currentAnimation >> "actions")) >> "Stop"));
                                    _this set [4, _animation];

                                    if ((_currentAnimation isNotEqualTo _animation) && !(_animation in (flatten (getAnimationsQueue _unit)))) then {
                                        _unit playActionNow "Stop";
                                    };
                                };
                            };
                        };
                    },
                    true,
                    0,
                    false
                ] call KH_fnc_execute
            ];
        },
        _unit,
        true,
        [
            "PERSISTENT", 
            _unit, 
            [_unit], 
            {
                params ["_unit"];
                _unit enableAIFeature ["ANIM", _unit getVariable ["KH_var_preUnitControllerAnimationFeature", true]];
                [_unit getVariable ["KH_var_unitControllerHandler", []]] call KH_fnc_removeHandler;
            }, 
            ""
        ]
    ] call KH_fnc_execute,
    [
        ["ENTITY", _unit, "PERSISTENT"],
        "PathCalculated",
        [_id, _condition],
        {
            params ["_unit", "_path"];
            _args params ["_id", "_condition"];

            if (((_unit getVariable ["KH_var_unitControllerId", ""]) isNotEqualTo _id) || !([_unit] call _condition) || !(missionNamespace getVariable _id)) exitWith {
                [_handlerId] call KH_fnc_removeHandler;
            };
            
            if !(isNull (_unit getVariable ["KH_var_pathObjectDestination", objNull])) then {
                _path set [-1, _unit getVariable ["KH_var_pathObjectDestination", objNull]];
                _unit setVariable ["KH_var_pathObjectDestination", objNull];
            };

            _unit setVariable ["KH_var_calculatedPath", _path];
            _unit setVariable ["KH_var_calculatedPathId", generateUid];
        }
    ] call KH_fnc_addEventhandler,
    [missionNamespace, _id, true]
];