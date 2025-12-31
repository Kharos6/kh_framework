params [
    ["_entity", objNull, [objNull]], 
    ["_target", objNull, [objNull]], 
    ["_position", [0, 0, 0], [true, []]], 
    ["_rotation", [0, 0, 0], [[]]],
    ["_scale", 1, [0]],
    ["_bone", "", [""]],
    ["_softAttach", false, [true]]
];

private _oldScale = getObjectScale _entity;

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

    _entity setVariable ["KH_var_attachLocalPosition", []];
    _entity setVariable ["KH_var_attachFallVelocity", 0];
    _entity setVariable ["KH_var_previousAttachFrameDirAndUp", []];
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

    if (_rotation isEqualTypeAll []) then {
        _entity setVectorDirAndUp _rotation;
    }
    else {
        _entity setRotationEuler _rotation;
    };

    private _healthHandler = if (_entity isKindOf "Man") then {
        [
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
    }
    else {
        [];
    };

    private _roadway = createVehicleLocal ["KH_WalkableSurface_1x1x0", getPosATLVisual _entity, [], 0, "CAN_COLLIDE"];

    private _attachHandler = [
        [_entity, _target, _scale, _oldScale, _attachHandlerId, _healthHandler, _roadway],
        {
            params ["_entity", "_target", "_scale", "_oldScale", "_attachHandlerId", "_healthHandler", "_roadway"];

            if !(missionNamespace getVariable _attachHandlerId) exitWith {
                deleteVehicle _roadway;
                _entity setPhysicsCollisionFlag true;
                _entity setVariable ["KH_var_attachLocalPosition", []];
                _entity setVariable ["KH_var_attachFallVelocity", 0];
                _entity setVariable ["KH_var_previousAttachFrameDirAndUp", []];
                _target setVariable ["KH_var_previousAttachFrameVelocity", []];

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
                
                [_healthHandler] call KH_fnc_removeHandler;
                [_handlerId] call KH_fnc_removeHandler;
            };

            private _deltaTime = diag_deltaTime;
            private _targetDir = vectorDirVisual _target;
            private _targetUp = vectorUpVisual _target;
            private _targetRight = _targetDir vectorCrossProduct _targetUp;
            private _entityDir = vectorDirVisual _entity;
            private _entityUp = vectorUpVisual _entity;
            private _targetVelocity = velocity _target;
            private _targetPosition = getPosASLVisual _target;
            private _entityPosition = getPosASLVisual _entity;
            private _entityAimPosition = AGLToASL (unitAimPositionVisual _entity);
            _entityAimPosition = [_entityPosition select 0, _entityPosition select 1, _entityAimPosition select 2];
            private _localPosition = _entity getVariable ["KH_var_attachLocalPosition", []];
            private _entityIgnores = [_entity, objectParent _entity, attachedTo _entity] + (attachedObjects _entity);

            private _roadwayIntersection = [
                _entityAimPosition,
                _entityPosition vectorAdd [0, 0, -999],
                _entityIgnores + [_roadway],
                true,
                1,
                "ROADWAY",
                "ROADWAY",
                true,
                []
            ] call KH_fnc_raycast;

            if (_roadwayIntersection isEqualTo []) then {
                _roadwayIntersection = [
                    _entityAimPosition,
                    _entityPosition vectorAdd [0, 0, -999],
                    _entityIgnores + [_roadway],
                    true,
                    1,
                    "GEOM",
                    "FIRE",
                    true,
                    []
                ] call KH_fnc_raycast;
            };

            if (_roadwayIntersection isNotEqualTo []) then {
                private _roadwayPosition = ((_roadwayIntersection select 0) select 0);
                _roadwayPosition set [2, (_roadwayPosition select 2) + 0.1];
                _roadway setPosASL _roadwayPosition;
            }
            else {
                _roadway setPosASL [0, 0, 0];
            };

            if (_localPosition isEqualTo []) then {
                private _relativePosition = _entityPosition vectorDiff _targetPosition;
                
                _localPosition = [
                    _relativePosition vectorDotProduct _targetRight,
                    _relativePosition vectorDotProduct _targetDir,
                    _relativePosition vectorDotProduct _targetUp
                ];

                _entity setVariable ["KH_var_attachLocalPosition", _localPosition];
            };

            private _movementVelocity = [0, 0, 0];
            
            if (_entity isKindOf "Man") then {
                private _movesInfo = getUnitMovesInfo _entity;

                if ((vectorMagnitude (_movesInfo select 4)) > 0) then {
                    private _localVelocity = (_movesInfo select 4) vectorMultiply (-1 / (_movesInfo select 2));
                    _movementVelocity = (((_entityDir vectorCrossProduct _entityUp) vectorMultiply (_localVelocity select 0)) vectorAdd (_entityDir vectorMultiply (_localVelocity select 1))) vectorAdd (_entityUp vectorMultiply (_localVelocity select 2));
                    private _worldMovement = _movementVelocity vectorMultiply _deltaTime;
                    _localPosition = _localPosition vectorAdd [_worldMovement vectorDotProduct _targetRight, _worldMovement vectorDotProduct _targetDir, _worldMovement vectorDotProduct _targetUp];
                    _entity setVariable ["KH_var_attachLocalPosition", _localPosition];
                };
            };

            private _groundIntersection = [
                _entityAimPosition,
                _entityPosition,
                _entityIgnores,
                true,
                1,
                "ROADWAY",
                "GEOM",
                true,
                []
            ] call KH_fnc_raycast;

            private _wallIntersections = [[
                [_entityPosition, _entityPosition vectorAdd [0, 0.5, 2], _entityIgnores, true, 1, "GEOM", "FIRE", true, []],
                [_entityPosition, _entityPosition vectorAdd [0, -0.5, 2], _entityIgnores, true, 1, "GEOM", "FIRE", true, []],
                [_entityPosition, _entityPosition vectorAdd [0.5, 0, 2], _entityIgnores, true, 1, "GEOM", "FIRE", true, []],
                [_entityPosition, _entityPosition vectorAdd [-0.5, 0, 2], _entityIgnores, true, 1, "GEOM", "FIRE", true, []],
                [_entityPosition, _entityPosition vectorAdd [0.354, 0.354, 2], _entityIgnores, true, 1, "GEOM", "FIRE", true, []],
                [_entityPosition, _entityPosition vectorAdd [-0.354, 0.354, 2], _entityIgnores, true, 1, "GEOM", "FIRE", true, []],
                [_entityPosition, _entityPosition vectorAdd [0.354, -0.354, 2], _entityIgnores, true, 1, "GEOM", "FIRE", true, []],
                [_entityPosition, _entityPosition vectorAdd [-0.354, -0.354, 2], _entityIgnores, true, 1, "GEOM", "FIRE", true, []]
            ]] call KH_fnc_raycast;

            private _wallCollisionVector = if (_wallIntersections isNotEqualTo []) then {
                private _totalVector = [0, 0, 0];

                {
                    private _currentVector = _x select 0;
                    private _currentPosition = [_entityPosition select 0, _entityPosition select 1, 0];
                    private _collisionPosition = [_currentVector select 0, _currentVector select 1, 0];
                    _totalVector = _totalVector vectorAdd ((_currentPosition vectorDiff _collisionPosition) vectorMultiply (1 - ((_currentPosition vectorDistance _collisionPosition) * 2)));
                } forEach _wallIntersections;

                _totalVector set [2, 0];
                _totalVector;
            }
            else {
                [0, 0, 0];
            };

            if (_wallCollisionVector isNotEqualTo [0, 0, 0]) then {
                _localPosition = _localPosition vectorAdd [_wallCollisionVector vectorDotProduct _targetRight, _wallCollisionVector vectorDotProduct _targetDir, _wallCollisionVector vectorDotProduct _targetUp];
                _entity setVariable ["KH_var_attachLocalPosition", _localPosition];
            };

            (_entity getVariable ["KH_var_previousAttachFrameDirAndUp", [_entityDir, _entityUp]]) params [["_previousVectorDir", _entityDir], ["_previousVectorUp", _entityUp]];
            
            private _correctionVelocity = if (_deltaTime > 0) then {
                ((((_targetRight vectorMultiply (_localPosition select 0)) vectorAdd (_targetDir vectorMultiply (_localPosition select 1))) vectorAdd (_targetUp vectorMultiply (_localPosition select 2))) vectorDiff (_entityPosition vectorDiff _targetPosition)) vectorMultiply (1 / _deltaTime);
            } else {
                [0, 0, 0];
            };

            private _fallVelocity = _entity getVariable ["KH_var_attachFallVelocity", 0];

            private _newPosition = if (_groundIntersection isEqualTo []) then {
                if (_entity isKindOf "Man") then {
                    if (isTouchingGround _entity) then {
                        _entity setVariable ["KH_var_attachFallVelocity", 0];
                        _entityPosition vectorAdd _wallCollisionVector;
                    }
                    else {
                        _fallVelocity = _fallVelocity - (9.807 * _deltaTime);
                        _entity setVariable ["KH_var_attachFallVelocity", _fallVelocity];
                        private _fallDisplacement = _fallVelocity * _deltaTime;
                        _localPosition set [2, (_localPosition select 2) + ([0, 0, _fallDisplacement] vectorDotProduct _targetUp)];
                        (_entityPosition vectorAdd _wallCollisionVector) vectorAdd [0, 0, _fallDisplacement];
                    };
                }
                else {
                    _entityPosition vectorAdd _wallCollisionVector;
                };
            }
            else {
                _entity setVariable ["KH_var_attachFallVelocity", 0];
                private _correctionZ = (((_groundIntersection select 0) select 0) select 2) - (_entityPosition select 2);
                _localPosition set [2, (_localPosition select 2) + ([0, 0, _correctionZ] vectorDotProduct _targetUp)];
                (_entityPosition vectorAdd [0, 0, _correctionZ]) vectorAdd _wallCollisionVector;
            };

            private _previousVelocity = if ((_target getVariable ["KH_var_previousAttachFrameVelocity", []]) isNotEqualTo []) then {
                _target getVariable ["KH_var_previousAttachFrameVelocity", []];
            }
            else {
                _targetVelocity;
            };

            _entity setVelocityTransformation [
                getPosASLVisual _entity, 
                _newPosition, 
                velocity _entity,
                (((_targetVelocity vectorAdd (_targetVelocity vectorDiff _previousVelocity)) vectorAdd _correctionVelocity) vectorAdd _movementVelocity) vectorAdd [0, 0, _fallVelocity], 
                _previousVectorDir, 
                vectorDirVisual _entity,
                _previousVectorUp,
                vectorUpVisual _entity,
                1
            ];

            if (_scale isEqualTo 0) then {
                _entity setObjectScale _oldScale;
            }
            else {
                _entity setObjectScale _scale;
            };

            _target setVariable ["KH_var_previousAttachFrameVelocity", velocity _target];
            _entity setVariable ["KH_var_previousAttachFrameDirAndUp", [vectorDirVisual _entity, vectorUpVisual _entity]];
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