params [
    ["_entity", objNull, [objNull]], 
    ["_target", objNull, [objNull]], 
    ["_position", true, [true, []]], 
    ["_rotation", true, [true, []]],
    ["_scale", 1, [0]],
    ["_bone", "", [""]],
    ["_softAttach", false, [true, []]]
];

private _oldScale = getObjectScale _entity;
private "_useGeometryRoadway";
private "_type";
private "_useGravity";
private "_attachDistance";

if (_softAttach isEqualType []) then {
    _useGeometryRoadway = _softAttach param [1, true, [true]];
    _type = _softAttach param [2, "VELOCITY", [""]];
    _useGravity = _softAttach param [3, true, [true]];
    _attachDistance = _softAttach param [4, 0, [0]];
    _softAttach = _softAttach param [0, true, [true]];
}
else {
    _useGeometryRoadway = true;
    _type = "VELOCITY";
    _useGravity = true;
    _attachDistance = 0;
};

if _softAttach exitWith {
    private _attachHandlerEntityId = ["KH_var_attachHandlerEntity_", hashValue _entity] joinString "";

    if ((missionNamespace getVariable [_attachHandlerEntityId, []]) isNotEqualTo []) then {
        [missionNamespace getVariable [_attachHandlerEntityId, []]] call KH_fnc_removeHandler;
        
        [
            [_entity, velocity _entity],
            {
                params ["_entity", "_velocity"];
                _entity setVelocity _velocity;
            },
            true,
            "-1",
            false
        ] call KH_fnc_execute;
    };

    private _attachHandlerId = generateUid;
    missionNamespace setVariable [_attachHandlerId, true];
    _entity setPhysicsCollisionFlag false;

    if (_position isEqualType true) then {
        _position = if _position then {
            getPosASLVisual _entity;
        }
        else {
            getPosASLVisual _target;
        };
    };

    if (_bone isNotEqualTo "") then {
        _entity setPosASL ((_target modelToWorldVisualWorld (_target selectionPosition _bone)) vectorAdd _position);
    }
    else {
        _entity setPosASL _position;
    };

    if (_rotation isEqualType true) then {
        _rotation = if _rotation then {
            [vectorDir _entity, vectorUp _entity];
        }
        else {
            [vectorDir _target, vectorUp _target];
        };
    };

    if (_rotation isEqualTypeAll []) then {
        _entity setVectorDirAndUp _rotation;
    }
    else {
        _entity setRotationEuler _rotation;
    };

    private _healthHandler = [
        ["ENTITY", _entity, "PERSISTENT"],
        "HandleDamage",
        [],
        {
            private _unit = param [0];
            private _projectile = param [4];
            private _hitPoint = param [7];
            private _context = param [9];

            if (_projectile isEqualTo "") then {
                if ((_context isEqualTo 0) || (_context isEqualTo 4)) then {
                    damage _unit;
                }
                else {
                    _unit getHitPointDamage _hitPoint;
                };
            };
        }
    ] call KH_fnc_addEventHandler;

    private _roadways = if _useGeometryRoadway then {
        [
            createVehicleLocal ["KH_InvisibleWalkableSurface_1x1x0", getPosATLVisual _entity, [], 0, "CAN_COLLIDE"],
            createVehicleLocal ["KH_InvisibleWalkableSurface_1x1x0", getPosATLVisual _entity, [], 0, "CAN_COLLIDE"]
        ];
    }
    else {
        [];
    };

    _entity setVariable ["KH_var_attachLocalPosition", []];
    _entity setVariable ["KH_var_attachFallVelocity", (velocity _entity) select 2];
    _entity setVariable ["KH_var_attachCurrentAnimationState", ""];
    _entity setVariable ["KH_var_attachAnimationBlend", 0];
    _entity setVariable ["KH_var_attachPreviousMovementVelocity", []];
    _entity setVariable ["KH_var_attachCurrentMovementVelocity", []];
    _entity setVariable ["KH_var_targetAttachVelocity", []];
    _entity setVariable ["KH_var_targetAttachDirection", getDir _target];

    private _attachHandler = [
        [_entity, _target, _scale, _oldScale, _attachHandlerId, _healthHandler, _roadways, _useGeometryRoadway, _type, _useGravity, _attachDistance],
        {
            params ["_entity", "_target", "_scale", "_oldScale", "_attachHandlerId", "_healthHandler", "_roadways", "_useGeometryRoadway", "_type", "_useGravity", "_attachDistance"];
            private _targetVelocity = velocity _target;
            private _targetPosition = getPosASLVisual _target;
            private _entityPosition = getPosASLVisual _entity;
            private _entityAimPosition = [_entityPosition select 0, _entityPosition select 1, (AGLToASL (unitAimPositionVisual _entity)) select 2];
            private _localPosition = _entity getVariable ["KH_var_attachLocalPosition", []];
            private _entityIgnores = [_entity] + (attachedObjects _entity);

            private _attachValid = if (_attachDistance isNotEqualTo 0) then {
                private _attachCheck = [
                    _entityAimPosition,
                    _entityPosition vectorAdd [0, 0, -_attachDistance],
                    _entityIgnores + ["TERRAIN", "KH_InvisibleWalkableSurface_1x1x0"],
                    true,
                    1,
                    "ROADWAY",
                    "NONE",
                    true,
                    []
                ] call KH_fnc_raycast;

                if (_attachCheck isNotEqualTo []) then {
                    (((_attachCheck select 0) select 3) isEqualTo _target);
                }
                else {
                    if _useGeometryRoadway then {
                        _attachCheck = [
                            _entityAimPosition,
                            _entityPosition vectorAdd [0, 0, -_attachDistance],
                            _entityIgnores + ["TERRAIN", "KH_InvisibleWalkableSurface_1x1x0"],
                            true,
                            1,
                            "GEOM",
                            "FIRE",
                            true,
                            []
                        ] call KH_fnc_raycast;

                        if (_attachCheck isNotEqualTo []) then {
                            (((_attachCheck select 0) select 3) isEqualTo _target);
                        }
                        else {
                            false;
                        };
                    }
                    else {
                        false;
                    };
                };
            }
            else {
                true;
            };
            
            if (!_attachValid || !(missionNamespace getVariable _attachHandlerId) || !(isNull (objectParent _entity)) || !(isNull (attachedTo _entity)) || !(alive _entity)) exitWith {
                {
                    deleteVehicle _x;
                } forEach _roadways;
        
                _entity setPhysicsCollisionFlag true;
                _entity setVariable ["KH_var_attachLocalPosition", []];
                _entity setVariable ["KH_var_attachFallVelocity", 0];
                _entity setVariable ["KH_var_attachCurrentAnimationState", ""];
                _entity setVariable ["KH_var_attachAnimationBlend", 0];
                _entity setVariable ["KH_var_attachPreviousMovementVelocity", []];
                _entity setVariable ["KH_var_attachCurrentMovementVelocity", []];
                _entity setVariable ["KH_var_targetAttachVelocity", []];
                _entity setVariable ["KH_var_targetAttachDirection", 0];                
                [_healthHandler] call KH_fnc_removeHandler;
                [_handlerId] call KH_fnc_removeHandler;
            };

            if (_localPosition isEqualTo []) then {
                _localPosition = _target vectorWorldToModelVisual (_entityPosition vectorDiff _targetPosition);
            };

            private _movementVelocity = [0, 0, 0];
            
            if (_entity isKindOf "Man") then {
                private _movesInfo = getUnitMovesInfo _entity;
                private _currentMovementVelocity = _entity vectorModelToWorldVisual ((_movesInfo select 4) vectorMultiply (-1 / (_movesInfo select 2)));
                
                if ((animationState _entity) isEqualTo (_entity getVariable ["KH_var_attachCurrentAnimationState", ""])) then {
                    _entity setVariable ["KH_var_attachAnimationBlend", ((_entity getVariable ["KH_var_attachAnimationBlend", 0]) + (_totalDelta * (getNumber (configFile >> (getText ((configOf _entity) >> "moves")) >> "states" >> (animationState _entity) >> "interpolationSpeed")))) min 1];
                }
                else {
                    _entity setVariable ["KH_var_attachCurrentAnimationState", animationState _entity];
                    _entity setVariable ["KH_var_attachPreviousMovementVelocity", _entity getVariable ["KH_var_attachCurrentMovementVelocity", [0, 0, 0]]];
                    _entity setVariable ["KH_var_attachAnimationBlend", 0];
                };
                
                _entity setVariable ["KH_var_attachCurrentMovementVelocity", _currentMovementVelocity];
                private _blend = _entity getVariable ["KH_var_attachAnimationBlend", 0];
                _movementVelocity = ((_entity getVariable ["KH_var_attachPreviousMovementVelocity", [0, 0, 0]]) vectorMultiply (1 - _blend)) vectorAdd (_currentMovementVelocity vectorMultiply _blend);
                _localPosition = _localPosition vectorAdd (_target vectorWorldToModelVisual (_movementVelocity vectorMultiply _totalDelta));
            };

            private _wallIntersectionStartPosition = _entityPosition vectorAdd [0, 0, 0.1];

            private _wallIntersections = [[
                [_wallIntersectionStartPosition, _entityAimPosition vectorAdd [0, 0.5, 1], _entityIgnores, true, 1, "GEOM", "FIRE", true, []],
                [_wallIntersectionStartPosition, _entityAimPosition vectorAdd [0, -0.5, 1], _entityIgnores, true, 1, "GEOM", "FIRE", true, []],
                [_wallIntersectionStartPosition, _entityAimPosition vectorAdd [0.5, 0, 1], _entityIgnores, true, 1, "GEOM", "FIRE", true, []],
                [_wallIntersectionStartPosition, _entityAimPosition vectorAdd [-0.5, 0, 1], _entityIgnores, true, 1, "GEOM", "FIRE", true, []],
                [_wallIntersectionStartPosition, _entityAimPosition vectorAdd [0.354, 0.354, 1], _entityIgnores, true, 1, "GEOM", "FIRE", true, []],
                [_wallIntersectionStartPosition, _entityAimPosition vectorAdd [-0.354, 0.354, 1], _entityIgnores, true, 1, "GEOM", "FIRE", true, []],
                [_wallIntersectionStartPosition, _entityAimPosition vectorAdd [0.354, -0.354, 1], _entityIgnores, true, 1, "GEOM", "FIRE", true, []],
                [_wallIntersectionStartPosition, _entityAimPosition vectorAdd [-0.354, -0.354, 1], _entityIgnores, true, 1, "GEOM", "FIRE", true, []],
                [_wallIntersectionStartPosition, _entityAimPosition vectorAdd [0.427, 0.427, 1], _entityIgnores, true, 1, "GEOM", "FIRE", true, []],
                [_wallIntersectionStartPosition, _entityAimPosition vectorAdd [-0.427, 0.427, 1], _entityIgnores, true, 1, "GEOM", "FIRE", true, []],
                [_wallIntersectionStartPosition, _entityAimPosition vectorAdd [0.427, -0.427, 1], _entityIgnores, true, 1, "GEOM", "FIRE", true, []],
                [_wallIntersectionStartPosition, _entityAimPosition vectorAdd [-0.427, -0.427, 1], _entityIgnores, true, 1, "GEOM", "FIRE", true, []],
                [_wallIntersectionStartPosition, _entityAimPosition vectorAdd [0.177, 0.177, 1], _entityIgnores, true, 1, "GEOM", "FIRE", true, []],
                [_wallIntersectionStartPosition, _entityAimPosition vectorAdd [-0.177, 0.177, 1], _entityIgnores, true, 1, "GEOM", "FIRE", true, []],
                [_wallIntersectionStartPosition, _entityAimPosition vectorAdd [0.177, -0.177, 1], _entityIgnores, true, 1, "GEOM", "FIRE", true, []],
                [_wallIntersectionStartPosition, _entityAimPosition vectorAdd [-0.177, -0.177, 1], _entityIgnores, true, 1, "GEOM", "FIRE", true, []]
            ]] call KH_fnc_raycast;

            private _wallCollisionVector = if (_wallIntersections isNotEqualTo []) then {
                private _totalVector = [0, 0, 0];

                {
                    _x params ["_hitPosition", "_hitNormal"];
                    private _entityPosition2d = [_entityPosition select 0, _entityPosition select 1, 0];
                    private _hitPosition2d = [_hitPosition select 0, _hitPosition select 1, 0];
                    private _penetration = 0.25 - (_entityPosition2d vectorDistance _hitPosition2d);
                    
                    if (_penetration > 0) then {
                        private _normal2d = [_hitNormal select 0, _hitNormal select 1, 0];
                        
                        private _pushDirection = if ((vectorMagnitude _normal2d) > 0.001) then {
                            vectorNormalized _normal2d;
                        } 
                        else {
                            private _rawDirection = _entityPosition2d vectorDiff _hitPosition2d;
                            
                            if ((vectorMagnitude _rawDirection) > 0.001) then {
                                vectorNormalized _rawDirection;
                            } else {
                                [0, 0, 0];
                            };
                        };
                        
                        _totalVector = _totalVector vectorAdd (_pushDirection vectorMultiply _penetration);
                    };
                } forEach _wallIntersections;

                _totalVector;
            }
            else {
                [0, 0, 0];
            };

            if (_wallCollisionVector isNotEqualTo [0, 0, 0]) then {
                _localPosition = _localPosition vectorAdd (_target vectorWorldToModelVisual _wallCollisionVector);
            };
            
            private _correctionVelocity = if (_totalDelta > 0) then {
                ((_targetPosition vectorAdd (_target vectorModelToWorldVisual _localPosition)) vectorDiff _entityPosition) vectorMultiply (1 / _totalDelta);
            }
            else {
                [0, 0, 0];
            };

            private _previousVelocity = if ((_entity getVariable ["KH_var_targetAttachVelocity", []]) isNotEqualTo []) then {
                _entity getVariable ["KH_var_targetAttachVelocity", []];
            }
            else {
                _targetVelocity;
            };

            private _newVelocity = ((_targetVelocity vectorAdd (_targetVelocity vectorDiff _previousVelocity)) vectorAdd _correctionVelocity) vectorAdd _movementVelocity;
            private _fallVelocity = _entity getVariable ["KH_var_attachFallVelocity", 0];

            if _useGeometryRoadway then {
                private _roadwayIntersectionA = [
                    _entityAimPosition,
                    _entityPosition vectorAdd [0, 0, -1],
                    _entityIgnores + _roadways + ["TERRAIN"],
                    true,
                    1,
                    "ROADWAY",
                    "NONE",
                    true,
                    []
                ] call KH_fnc_raycast;

                if (_roadwayIntersectionA isEqualTo []) then {
                    _roadwayIntersectionA = [
                        _entityAimPosition,
                        _entityPosition vectorAdd [0, 0, -1],
                        _entityIgnores + _roadways + ["TERRAIN"],
                        true,
                        1,
                        "GEOM",
                        "FIRE",
                        true,
                        []
                    ] call KH_fnc_raycast;
                };

                if (_roadwayIntersectionA isNotEqualTo []) then {
                    (_roadways select 0) setPosASL (((_roadwayIntersectionA select 0) select 0) vectorAdd (((_roadwayIntersectionA select 0) select 1) vectorMultiply 0.01));
                }
                else {
                    (_roadways select 0) setPosASL [0, 0, 0];
                };

                private _roadwayIntersectionB = [
                    _entityAimPosition vectorAdd (_newVelocity vectorMultiply _totalDelta),
                    (_entityPosition vectorAdd [0, 0, -1]) vectorAdd (_newVelocity vectorMultiply _totalDelta),
                    _entityIgnores + _roadways + ["TERRAIN"],
                    true,
                    1,
                    "ROADWAY",
                    "NONE",
                    true,
                    []
                ] call KH_fnc_raycast;

                if (_roadwayIntersectionB isEqualTo []) then {
                    _roadwayIntersectionB = [
                        _entityAimPosition vectorAdd (_newVelocity vectorMultiply _totalDelta),
                        (_entityPosition vectorAdd [0, 0, -1]) vectorAdd (_newVelocity vectorMultiply _totalDelta),
                        _entityIgnores + _roadways + ["TERRAIN"],
                        true,
                        1,
                        "GEOM",
                        "FIRE",
                        true,
                        []
                    ] call KH_fnc_raycast;
                };

                if (_roadwayIntersectionB isNotEqualTo []) then {
                    (_roadways select 1) setPosASL (((_roadwayIntersectionB select 0) select 0) vectorAdd (((_roadwayIntersectionB select 0) select 1) vectorMultiply 0.01));
                }
                else {
                    (_roadways select 1) setPosASL [0, 0, 0];
                };
            };

            private _groundIntersection = [
                _entityAimPosition,
                _entityPosition,
                _entityIgnores,
                true,
                1,
                "ROADWAY",
                "NONE",
                true,
                []
            ] call KH_fnc_raycast;

            private _velocityBased = switch _type do {
                case "VELOCITY": {
                    true;
                };

                case "POSITION": {
                    false;
                };

                case "VARIABLE": {
                    (vectorMagnitude _newVelocity) < 10;
                };
            };

            private _newPosition = if (_useGravity && (_groundIntersection isEqualTo [])) then {
                if (_entity isKindOf "Man") then {
                    if (isTouchingGround _entity) then {
                        _fallVelocity = 0;

                        if _velocityBased then {
                            _newVelocity set [2, (_newVelocity select 2) - 0.05];
                        };
                    }
                    else {
                        _fallVelocity = ((_fallVelocity - (0.5 * 9.807 * _totalDelta)) min -0.01) max -60;

                        if _velocityBased then {
                            _newVelocity set [2, ((_newVelocity select 2) + _fallVelocity) max -60];
                        };

                        _localPosition = _localPosition vectorAdd (_target vectorWorldToModelVisual [0, 0, _fallVelocity * _totalDelta]);
                    };
                }
                else {
                    _fallVelocity = ((_fallVelocity - (0.5 * 9.807 * _totalDelta)) min -0.01) max -60;

                    if _velocityBased then {
                        _newVelocity set [2, ((_newVelocity select 2) + _fallVelocity) max -60];
                    };

                    _localPosition = _localPosition vectorAdd (_target vectorWorldToModelVisual [0, 0, _fallVelocity * _totalDelta]);
                };

                [];
            }
            else {
                _fallVelocity = 0;
                private _correctionZ = (((_groundIntersection select 0) select 0) select 2) - (_entityPosition select 2);
                _localPosition = _localPosition vectorAdd (_target vectorWorldToModelVisual [0, 0, _correctionZ]);
                _entityPosition vectorAdd [0, 0, _correctionZ];
            };

            private _lifeState = lifeState _entity;

            if (_velocityBased && ((_lifeState isNotEqualTo "INCAPACITATED") && (_lifeState isNotEqualTo "UNCONSCIOUS") && !(_entity getVariable ["KH_var_incapacitated", false]))) then {
                if (_newPosition isNotEqualTo []) then {
                    _entity setPosASL _newPosition;
                };

                _entity setVelocity _newVelocity;
            }
            else {
                if (_newPosition isNotEqualTo []) then {
                    _entity setPosASL (_newPosition vectorAdd (_newVelocity vectorMultiply _totalDelta));
                }
                else {
                    _entity setPosASL (_entityPosition vectorAdd (_newVelocity vectorMultiply _totalDelta));
                };

                _entity setVelocity [0, 0, _fallVelocity];
            };

            private _direction = (getDir _entity) - ((_entity getVariable ["KH_var_targetAttachDirection", getDir _target]) - (getDir _target));
            _entity setVectorDir [sin _direction, cos _direction, 0];

            if (_scale isEqualTo 0) then {
                _entity setObjectScale _oldScale;
            }
            else {
                _entity setObjectScale _scale;
            };

            _entity setVariable ["KH_var_attachFallVelocity", _fallVelocity];
            _entity setVariable ["KH_var_attachLocalPosition", _localPosition];
            _entity setVariable ["KH_var_targetAttachDirection", getDir _target];
            _entity setVariable ["KH_var_targetAttachVelocity", velocity _target];
        },
        true,
        0,
        false
    ] call KH_fnc_execute;

    missionNamespace setVariable [_attachHandlerEntityId, [_healthHandler, _attachHandler]];
    [missionNamespace, _attachHandlerId, clientOwner];
};

if (_position isEqualType true) then {
    _position = if _position then {
        _target worldToModelVisual (_entity modelToWorldVisual [0, 0, 0]);
    }
    else {
        [0, 0, 0];
    };
};

_entity attachTo [_target, _position, _bone, true];

if (_rotation isEqualTypeAll []) then {
    _entity setVectorDirAndUp _rotation;
}
else {
    _entity setRotationEuler _rotation;
};

if (_scale isEqualTo 0) then {
    _entity setObjectScale _oldScale;
}
else {
    _entity setObjectScale _scale;
};