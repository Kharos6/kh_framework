params [["_unit", objNull, [objNull]], ["_captureRespawn", true, [true]]];
private _name = ["KH_var_collectedData_", CBA_missionTime] joinString "";
_unit setVariable [_name, createHashMap];
private _data = _unit getVariable _name;

if !(_unit getVariable ["KH_var_allowDataCollection", true]) exitWith {
    [
        _unit getVariable ["KH_var_allCollectedData", []],
        _data,
        []
    ];
};

if ((_unit getVariable ["KH_var_dataCollectionHandlers", []]) isNotEqualTo []) then {
    [_unit getVariable ["KH_var_dataCollectionHandlers", []]] call KH_fnc_removeHandler;
    _unit setVariable ["KH_var_dataCollectionHandlers", []];
};

if (_unit isNil "KH_var_allCollectedData") then {
    _unit setVariable ["KH_var_allCollectedData", []];
};

(_unit getVariable ["KH_var_allCollectedData", []]) pushBack [CBA_missionTime, _data];
_data set ["Animation", []];
_data set ["Burning", []];
_data set ["Captive", []];
_data set ["Damage", []];
_data set ["Deleted", -1];
_data set ["Direction", []];
_data set ["EyeDirection", []];
_data set ["Fatigue", []];
_data set ["Fire", []];
_data set ["FlashlightState", []];
_data set ["GetIn", []];
_data set ["GetOut", []];
_data set ["Gesture", []];
_data set ["Group", []];

_data set [
    "Header", 
    [ 
        netId _unit,
        getPlayerUID _unit,
        roleDescription _unit,
        [_unit] call KH_fnc_getUnitAttributes
    ]
];

_data set ["Heal", []];
_data set ["Hit", []];
_data set ["Incapacitation", []];
_data set ["InsideBuilding", []];
_data set ["InventoryClosed", []];
_data set ["InventoryOpened", []];
_data set ["IRLaserState", []];
_data set ["Killed", -1];
_data set ["LaserState", []];
_data set ["Leaning", []];
_data set ["OnRoad", []];
_data set ["Oxygen", []];
_data set ["Position", []];
_data set ["ProjectileExplode", []];
_data set ["ProjectileHit", []];
_data set ["ProjectileHitExplode", []];
_data set ["Put", []];
_data set ["Reload", []];
_data set ["SeatSwitch", []];
_data set ["SlotItemChange", []];
_data set ["Stance", []];
_data set ["Suppressed", []];
_data set ["Take", []];
_data set ["Targets", []];
_data set ["Task", []];
_data set ["Team", []];
_data set ["VehicleTurnState", []];
_data set ["Underwater", []];
_data set ["ValidPositionsExterior", []];
_data set ["ValidPositionsInterior", []];
_data set ["Velocity", []];
_data set ["VisionMode", []];
_data set ["Weapon", [[CBA_missionTime, [currentWeapon _unit, currentWeaponMode _unit, currentMuzzle _unit]]]];
_data set ["WeaponDirection", []];
_data set ["WeaponLowered", []];

if !(isNull (objectParent _unit)) then {
    private _vehicleRole = assignedVehicleRole _unit;
    (_data get "GetIn") pushBack [CBA_missionTime, [_vehicleRole param [0, ""], netId (objectParent _unit), _vehicleRole param [1, []]]];
};

private _result = [
    _unit getVariable ["KH_var_allCollectedData", []],
    _data,
    [
        [
            ["ENTITY", _unit, "LOCAL"],
            "Deleted",
            [_data],
            {
                params ["_unit"];
                _args params ["_data"];
                _data set ["Deleted", CBA_missionTime];
            }
        ] call KH_fnc_addEventHandler,
        [
            ["ENTITY", _unit, "PERSISTENT"],
            "FiredMan",
            [clientOwner, _name],
            {
                params ["_unit", "_weapon", "_muzzle", "_mode", "_ammo", "_magazine", "_projectile", "_vehicle"];
                _args params ["_collector", "_name"];

                [
                    ["ENTITY", _projectile, "LOCAL"],
                    "Explode",
                    [_unit, _collector, _name],
                    {
                        params ["_projectile", "_position", "_velocity"];
                        _args params ["_unit", "_collector", "_name"];

                        [
                            [_projectile, _position, _velocity, _unit],
                            {
                                params ["_projectile", "_position", "_velocity", "_unit"];
                                ((_unit getVariable _name) get "ProjectileExplode") pushBack [(CBA_missionTime - ([_unit, false] call KH_fnc_getLatency)) max 0, [netId _projectile, _position, _velocity]];
                            },
                            _collector,
                            true,
                            false
                        ] call KH_fnc_execute;
                    }
                ] call KH_fnc_addEventHandler;

                [
                    ["ENTITY", _projectile, "LOCAL"],
                    "HitPart",
                    [_unit, _collector, _name],
                    {
                        private _projectile = param [0];
                        private _hitEntity = param [1];
                        private _position = param [3];
                        private _velocity = param [4];
                        _args params ["_unit", "_collector", "_name"];

                        [
                            [_projectile, _hitEntity, _position, _velocity, _unit],
                            {
                                params ["_projectile", "_hitEntity", "_position", "_velocity", "_unit"];
                                ((_unit getVariable _name) get "ProjectileHit") pushBack [(CBA_missionTime - ([_unit, false] call KH_fnc_getLatency)) max 0, [netId _hitEntity, netId _projectile, _position, _velocity]];
                            },
                            _collector,
                            true,
                            false
                        ] call KH_fnc_execute;
                    }
                ] call KH_fnc_addEventHandler;

                [
                    ["ENTITY", _projectile, "LOCAL"],
                    "HitExplosion",
                    [_unit, _collector, _name],
                    {
                        params ["_projectile", "_hitEntity"];
                        _args params ["_unit", "_collector", "_name"];

                        [
                            [_projectile, _hitEntity, _unit],
                            {
                                params ["_projectile", "_hitEntity", "_unit"];
                                ((_unit getVariable _name) get "ProjectileHitExplode") pushBack [(CBA_missionTime - ([_unit, false] call KH_fnc_getLatency)) max 0, [netId _hitEntity, netId _projectile]];
                            },
                            _collector,
                            true,
                            false
                        ] call KH_fnc_execute;
                    }
                ] call KH_fnc_addEventHandler;
                
                [
                    [_unit, _name, _weapon, _muzzle, _mode, _ammo, _magazine, _projectile, _vehicle],
                    {
                        params ["_unit", "_name", "_weapon", "_muzzle", "_mode", "_ammo", "_magazine", "_projectile", "_vehicle"];
                        ((_unit getVariable _name) get "Fire") pushBack [(CBA_missionTime - ([_unit, false] call KH_fnc_getLatency)) max 0, [_weapon, _muzzle, _mode, _ammo, _magazine, netId _projectile, velocity _projectile, netId _vehicle]];
                    },
                    _collector,
                    true,
                    false
                ] call KH_fnc_execute;

                nil;
            }
        ] call KH_fnc_addEventHandler,
        [
            ["ENTITY", _unit, "REMOTE"],
            "GetInMan",
            [_data],
            {
                params ["_unit", "_role", "_vehicle", "_turret"];
                _args params ["_data"];
                (_data get "GetIn") pushBack [(CBA_missionTime - ([_unit, false] call KH_fnc_getLatency)) max 0, [_role, netId _vehicle, _turret]];
            }
        ] call KH_fnc_addEventHandler,
        [
            ["ENTITY", _unit, "REMOTE"],
            "GetOutMan",
            [_data],
            {
                params ["_unit", "_role", "_vehicle", "_turret", "_isEject"];
                _args params ["_data"];
                (_data get "GetOut") pushBack [(CBA_missionTime - ([_unit, false] call KH_fnc_getLatency)) max 0, [_role, netId _vehicle, _turret, _isEject]];
            }
        ] call KH_fnc_addEventHandler,
        [
            ["ENTITY", _unit, "REMOTE"],
            "HandleHeal",
            [_unit, _data],
            {
                params ["_injured", "_healer", "_isMedic"];
                _args params ["_unit", "_data"];
                (_data get "Heal") pushBack [(CBA_missionTime - ([_unit, false] call KH_fnc_getLatency)) max 0, [netId _injured, netId _healer, _isMedic]];
            }
        ] call KH_fnc_addEventHandler,
        [
            ["ENTITY", _unit, "REMOTE"],
            "HandleDamage",
            [_data],
            {
                private _unit = param [0];
                private _source = param [3];
                private _projectile = param [4];
                private _instigator = param [6];
                private _hitPoint = param [7];
                _args params ["_data"];
                (_data get "Hit") pushBack [(CBA_missionTime - ([_unit, false] call KH_fnc_getLatency)) max 0, [netId _source, _projectile, netId _instigator, _hitPoint]];
            }
        ] call KH_fnc_addEventHandler,
        [
            ["ENTITY", _unit, "REMOTE"],
            "InventoryClosed",
            [_data],
            {
                params ["_unit", "_container"];
                _args params ["_data"];
                (_data get "InventoryClosed") pushBack [(CBA_missionTime - ([_unit, false] call KH_fnc_getLatency)) max 0, netId _container];
            }
        ] call KH_fnc_addEventHandler,
        [
            ["ENTITY", _unit, "REMOTE"],
            "InventoryOpened",
            [_data],
            {
                params ["_unit", "_container"];
                _args params ["_data"];
                (_data get "InventoryOpened") pushBack [(CBA_missionTime - ([_unit, false] call KH_fnc_getLatency)) max 0, netId _container];
            }
        ] call KH_fnc_addEventHandler,
        [
            ["ENTITY", _unit, "REMOTE"],
            "Killed",
            [_data],
            {
                params ["_unit"];
                _args params ["_data"];
                _data set ["Killed", (CBA_missionTime - ([_unit, false] call KH_fnc_getLatency)) max 0];
            }
        ] call KH_fnc_addEventHandler,
        [
            ["ENTITY", _unit, "REMOTE"],
            "LeaningChanged",
            [_data],
            {
                params ["_unit", "_newLeaning", "_oldLeaning"];
                _args params ["_data"];
                (_data get "Leaning") pushBack [(CBA_missionTime - ([_unit, false] call KH_fnc_getLatency)) max 0, [_newLeaning, _oldLeaning]];
            }
        ] call KH_fnc_addEventHandler,
        [
            ["ENTITY", _unit, "REMOTE"],
            "Put",
            [_data],
            {
                params ["_unit", "_container", "_item"];
                _args params ["_data"];
                (_data get "Put") pushBack [(CBA_missionTime - ([_unit, false] call KH_fnc_getLatency)) max 0, [netId _container, _item]];
            }
        ] call KH_fnc_addEventHandler,
        [
            ["ENTITY", _unit, "REMOTE"],
            "Reloaded",
            [_data],
            {
                params ["_unit", "_weapon", "_muzzle", "_newMagazine"];
                _args params ["_data"];
                (_data get "Reload") pushBack [(CBA_missionTime - ([_unit, false] call KH_fnc_getLatency)) max 0, [_weapon, _muzzle, _newMagazine]];
            }
        ] call KH_fnc_addEventHandler,
        [
            ["ENTITY", _unit, "PERSISTENT"],
            "Respawn",
            [_captureRespawn],
            {
                params ["_unit"];
                _args params ["_captureRespawn"];
                
                if _captureRespawn then {
                    [_unit] call KH_fnc_collectUnitData;
                }
                else {
                    [_unit getVariable ["KH_var_dataCollectionHandlers", []]] call KH_fnc_removeHandler;
                };

                nil;
            }
        ] call KH_fnc_addEventHandler,
        [
            ["ENTITY", _unit, "REMOTE"],
            "SlotItemChanged",
            [_data],
            {
                params ["_unit", "_name", "_slot", "_assigned", "_weapon"];
                _args params ["_data"];
                (_data get "SlotItemChange") pushBack [(CBA_missionTime - ([_unit, false] call KH_fnc_getLatency)) max 0, [_name, _slot, _assigned, _weapon]];
            }
        ] call KH_fnc_addEventHandler,
        [
            ["ENTITY", _unit, "REMOTE"],
            "SeatSwitchedMan",
            [_data],
            {
                params ["_unit", "_secondUnit", "_vehicle"];
                _args params ["_data"];
                (_data get "SeatSwitch") pushBack [(CBA_missionTime - ([_unit, false] call KH_fnc_getLatency)) max 0, [netId _secondUnit, netId _vehicle, assignedVehicleRole _unit]];
            }
        ] call KH_fnc_addEventHandler,
        [
            ["ENTITY", _unit, "REMOTE"],
            "Suppressed",
            [_data],
            {
                private _unit = param [0];
                private _distance = param [1];
                private _instigator = param [3];
                private _ammoObject = param [4];
                private _ammoClassName = param [5];
                _args params ["_data"];
                (_data get "Suppressed") pushBack [(CBA_missionTime - ([_unit, false] call KH_fnc_getLatency)) max 0, [_distance, netId _instigator, netId _ammoObject, _ammoClassName]];
            }
        ] call KH_fnc_addEventHandler,
        [
            ["ENTITY", _unit, "REMOTE"],
            "Take",
            [_data],
            {
                params ["_unit", "_container", "_item"];
                _args params ["_data"];
                (_data get "Take") pushBack [(CBA_missionTime - ([_unit, false] call KH_fnc_getLatency)) max 0, [netId _container, _item]];
            }
        ] call KH_fnc_addEventHandler,
        [
            ["ENTITY", _unit, "REMOTE"],
            "VisionModeChanged",
            [_data],
            {
                private _unit = param [0];
                private _visionMode = param [1];
                private _TiIndex = param [2];
                private _vehicle = param [5];
                private _turret = param [6];
                _args params ["_data"];
                (_data get "VisionMode") pushBack [(CBA_missionTime - ([_unit, false] call KH_fnc_getLatency)) max 0, [_visionMode, _TiIndex, netId _vehicle, _turret]];
            }
        ] call KH_fnc_addEventHandler,
        [
            ["ENTITY", _unit, "REMOTE"],
            "WeaponChanged",
            [_data],
            {
                private _newWeapon = param [2];
                private _newMode = param [4];
                private _newMuzzle = param [6];
                private _turretIndex = param [7];
                _args params ["_data"];
                (_data get "Weapon") pushBack [(CBA_missionTime - ([_unit, false] call KH_fnc_getLatency)) max 0, [_newWeapon, _newMode, _newMuzzle, _turretIndex]];
            }
        ] call KH_fnc_addEventHandler,
        [
            [_unit, _data],
            {
                params ["_unit", "_data"];
                private _trueTime = (CBA_missionTime - ([_unit, false] call KH_fnc_getLatency)) max 0;

                if ((_unit getVariable ["KH_var_collectedAnimation", []]) isNotEqualTo [animationState _unit, _unit getUnitMovesInfo 4]) then {
                    private _currentAnimation = [animationState _unit, _unit getUnitMovesInfo 4];
                    (_data get "Animation") pushBack [_trueTime, _currentAnimation];
                    _unit setVariable ["KH_var_collectedAnimation", _currentAnimation];
                };

                if ((_unit getVariable ["KH_var_collectedBurning", false]) isNotEqualTo (isBurning _unit)) then {
                    private _currentBurning = isBurning _unit;
                    (_data get "Burning") pushBack [_trueTime, _currentBurning];
                    _unit setVariable ["KH_var_collectedBurning", _currentBurning];
                };

                if ((_unit getVariable ["KH_var_collectedCaptive", false]) isNotEqualTo (captive _unit)) then {
                    private _currentCaptive = captive _unit;
                    (_data get "Captive") pushBack [_trueTime, _currentCaptive];
                    _unit setVariable ["KH_var_collectedCaptive", _currentCaptive];
                };

                private _hitPointsDamage = getAllHitPointsDamage _unit;
                private _hitPointsDamageValues = _hitPointsDamage select 2;
                private _processedHitPointsDamage = [];

                {
                    _processedHitPointsDamage pushBack [_x, _hitPointsDamageValues select _forEachIndex];
                } forEach (_hitPointsDamage select 0);

                if ((_unit getVariable ["KH_var_collectedDamage", []]) isNotEqualTo [damage _unit, _processedHitPointsDamage]) then {
                    private _currentDamage = [damage _unit, _processedHitPointsDamage];
                    (_data get "Damage") pushBack [_trueTime, _currentDamage];
                    _unit setVariable ["KH_var_collectedDamage", _currentDamage];
                };

                if ((_unit getVariable ["KH_var_collectedDirection", []]) isNotEqualTo [vectorDirVisual _unit, vectorUpVisual _unit]) then {
                    private _currentDirection = [vectorDirVisual _unit, vectorUpVisual _unit];
                    (_data get "Direction") pushBack [_trueTime, _currentDirection];
                    _unit setVariable ["KH_var_collectedDirection", _currentDirection];
                };

                if ((_unit getVariable ["KH_var_collectedEyeDirection", []]) isNotEqualTo (eyeDirection _unit)) then {
                    private _currentEyeDirection = eyeDirection _unit;
                    (_data get "EyeDirection") pushBack [_trueTime, _currentEyeDirection];
                    _unit setVariable ["KH_var_collectedEyeDirection", _currentEyeDirection];
                };

                if ((_unit getVariable ["KH_var_collectedFatigue", -1]) isNotEqualTo (getFatigue _unit)) then {
                    private _currentFatigue = getFatigue _unit;
                    (_data get "Fatigue") pushBack [_trueTime, _currentFatigue];
                    _unit setVariable ["KH_var_collectedFatigue", _currentFatigue];
                };

                if ((_unit getVariable ["KH_var_collectedFlashlightState", false]) isNotEqualTo (_unit isFlashlightOn (currentWeapon _unit))) then {
                    private _currentFlashlightState = _unit isFlashlightOn (currentWeapon _unit);
                    (_data get "FlashlightState") pushBack [_trueTime, _currentFlashlightState];
                    _unit setVariable ["KH_var_collectedFlashlightState", _currentFlashlightState];
                };

                if ((_unit getVariable ["KH_var_collectedGesture", ""]) isNotEqualTo (gestureState _unit)) then {
                    private _currentGesture = gestureState _unit;
                    (_data get "Gesture") pushBack [_trueTime, _currentGesture];
                    _unit setVariable ["KH_var_collectedGesture", _currentGesture];
                };

                if ((_unit getVariable ["KH_var_collectedGroup", []]) isNotEqualTo (netId (group _unit))) then {
                    private _currentGroup = netId (group _unit);
                    (_data get "Group") pushBack [_trueTime, _currentGroup];
                    _unit setVariable ["KH_var_collectedGroup", _currentGroup];
                };

                if ((_unit getVariable ["KH_var_collectedIncapacitation", false]) isNotEqualTo (((lifeState _unit) isEqualTo "INCAPACITATED") || ((lifeState _unit) isEqualTo "UNCONSCIOUS"))) then {
                    private _incapacitated = ((lifeState _unit) isEqualTo "INCAPACITATED") || ((lifeState _unit) isEqualTo "UNCONSCIOUS");
                    (_data get "Incapacitation") pushBack [_trueTime, _incapacitated];
                    _unit setVariable ["KH_var_collectedIncapacitation", _incapacitated];
                };

                if ((_unit getVariable ["KH_var_collectedInsideBuilding", false]) isNotEqualTo (insideBuilding _unit)) then {
                    private _currentInsideBuilding = insideBuilding _unit;
                    (_data get "InsideBuilding") pushBack [_trueTime, _currentInsideBuilding];
                    _unit setVariable ["KH_var_collectedInsideBuilding", _currentInsideBuilding];
                };

                if ((_unit getVariable ["KH_var_collectedIrLaserState", false]) isNotEqualTo (_unit isIRLaserOn (currentWeapon _unit))) then {
                    private _currentIrLaserState = _unit isIRLaserOn (currentWeapon _unit);
                    (_data get "IRLaserState") pushBack [_trueTime, _currentIrLaserState];
                    _unit setVariable ["KH_var_collectedIrLaserState", _currentIrLaserState];
                };

                if ((_unit getVariable ["KH_var_collectedLaserState", false]) isNotEqualTo (isLaserOn _unit)) then {
                    private _currentLaserState = isLaserOn _unit;
                    (_data get "LaserState") pushBack [_trueTime, _currentLaserState];
                    _unit setVariable ["KH_var_collectedLaserState", _currentLaserState];
                };

                if ((_unit getVariable ["KH_var_collectedOnRoad", false]) isNotEqualTo (isOnRoad _unit)) then {
                    private _currentOnRoad = isOnRoad _unit;
                    (_data get "OnRoad") pushBack [_trueTime, _currentOnRoad];
                    _unit setVariable ["KH_var_collectedOnRoad", _currentOnRoad];
                };

                if ((_unit getVariable ["KH_var_collectedOxygen", -1]) isNotEqualTo (getOxygenRemaining _unit)) then {
                    private _currentOxygen = getOxygenRemaining _unit;
                    (_data get "Oxygen") pushBack [_trueTime, _currentOxygen];
                    _unit setVariable ["KH_var_collectedOxygen", _currentOxygen];
                };

                private _positionAsl = getPosASLVisual _unit;

                if ((_unit getVariable ["KH_var_collectedPosition", []]) isNotEqualTo [_positionAsl select [0, 2], (getPosATLVisual _unit) select 2, _positionAsl select 2, (getPosVisual _unit) select 2]) then {
                    private _currentPosition = [_positionAsl select [0, 2], (getPosATLVisual _unit) select 2, _positionAsl select 2, (getPosVisual _unit) select 2];
                    (_data get "Position") pushBack [CBA_missionTime, _currentPosition];
                    _unit setVariable ["KH_var_collectedPosition", _currentPosition];
                };

                if ((_unit getVariable ["KH_var_collectedStance", ""]) isNotEqualTo (toUpperANSI (unitPos _unit))) then {
                    private _currentStance = switch (stance _unit) do {
                        case "STAND": {
                            "UP";
                        };

                        case "CROUCH": {
                            "MIDDLE";
                        };

                        case "PRONE": {
                            "DOWN";
                        };

                        default {
                            "AUTO";
                        };
                    };

                    (_data get "Stance") pushBack [_trueTime, _currentStance];
                    _unit setVariable ["KH_var_collectedStance", _currentStance];
                };

                private _currentTargets = (_unit targetsQuery [objNull, sideUnknown, "", [], 0]) apply {
                    [_x select 0, netId (_x select 1), _x select 2, _x select 3, _x select 4, _x select 5];
                };

                _currentTargets = [_currentTargets, [], {_x select 5;}, "ASCEND"] call BIS_fnc_sortBy;

                if ((_unit getVariable ["KH_var_collectedTargets", []]) isNotEqualTo _currentTargets) then {
                    (_data get "Targets") pushBack [CBA_missionTime, _currentTargets];
                    _unit setVariable ["KH_var_collectedTargets", _currentTargets];
                };

                private _currentTask = if ((count (waypoints _unit)) > 1) then {
                    private _currentWaypoint = ((waypoints _unit) select {(_x select 1) isEqualTo (currentWaypoint (group _unit));}) select 0;
                    private _taskPosition = getWPPos [_unit, _currentWaypoint select 1];

                    private _roadwayTaskIntersection = [
                        _taskPosition vectorAdd [0, 0, 0.1],
                        _taskPosition vectorAdd [0, 0, -0.1],
                        ["TERRAIN"],
                        true,
                        1,
                        "ROADWAY",
                        "NONE",
                        true,
                        []
                    ] call KH_fnc_raycast;

                    private _object = objNull;

                    private _entries = if (_roadwayTaskIntersection isNotEqualTo []) then {
                        _object = (_roadwayTaskIntersection select 0) select 3;
                        private _currentEntries = [];
                        
                        for "_i" from 0 to 999 do {
                            private _entry = _object buildingExit _i;

                            if ((_entry isNotEqualTo []) && (_entry isNotEqualTo [0, 0, 0])) then {
                                _currentEntries pushBack _entry;
                            }
                            else {
                                break;
                            };
                        };

                        _currentEntries;
                    }
                    else {
                        _object = waypointAttachedVehicle [group _unit, currentWaypoint (group _unit)];

                        if (isNull _object) then {
                            _object = waypointAttachedObject [group _unit, currentWaypoint (group _unit)];

                            if (isNull _object) then {
                                [];
                            }
                            else {
                                private _currentEntries = [];
                                
                                for "_i" from 0 to 999 do {
                                    private _entry = _object buildingExit _i;

                                    if ((_entry isNotEqualTo []) && (_entry isNotEqualTo [0, 0, 0])) then {
                                        _currentEntries pushBack _entry;
                                    }
                                    else {
                                        break;
                                    };
                                };

                                _currentEntries;
                            };
                        }
                        else {
                            private _currentEntries = [];
                            
                            for "_i" from 0 to 999 do {
                                private _entry = _object buildingExit _i;

                                if ((_entry isNotEqualTo []) && (_entry isNotEqualTo [0, 0, 0])) then {
                                    _currentEntries pushBack _entry;
                                }
                                else {
                                    break;
                                };
                            };

                            _currentEntries;
                        };
                    };

                    [
                        waypointType _currentWaypoint, 
                        _taskPosition,
                        netId _object, 
                        if !(isNull _object) then {
                            _object call BIS_fnc_buildingPositions;
                        }
                        else {
                            [];
                        }, 
                        _entries
                    ];
                }
                else {
                    [];
                };

                private _otherTasks = (waypoints _unit) apply {[waypointType _x, getWPPos [_unit, _x select 1]];};

                if ((_unit getVariable ["KH_var_collectedTask", []]) isNotEqualTo [_currentTask, _otherTasks]) then {
                    (_data get "Task") pushBack [_trueTime, [_currentTask, _otherTasks]];
                    _unit setVariable ["KH_var_collectedTask", [_currentTask, _otherTasks]];
                };

                if ((_unit getVariable ["KH_var_collectedTeam", []]) isNotEqualTo (assignedTeam _unit)) then {
                    private _currentTeam = assignedTeam _unit;
                    (_data get "Team") pushBack [_trueTime, _currentTeam];
                    _unit setVariable ["KH_var_collectedTeam", _currentTeam];
                };

                if ((_unit getVariable ["KH_var_collectedUnderwater", false]) isNotEqualTo (((eyePos _unit) select 2) < 0)) then {
                    private _currentUnderwater = ((eyePos _unit) select 2) < 0;
                    (_data get "Underwater") pushBack [_trueTime, _currentUnderwater];
                    _unit setVariable ["KH_var_collectedUnderwater", _currentUnderwater];
                };

                private _roadwayIntersection = [
                    _positionAsl vectorAdd [0, 0, 0.1],
                    _positionAsl vectorAdd [0, 0, -0.1],
                    [_unit, (objectParent _unit), (attachedTo _unit), "TERRAIN"] + (attachedObjects _unit),
                    true,
                    1,
                    "ROADWAY",
                    "NONE",
                    true,
                    []
                ] call KH_fnc_raycast;

                if (_roadwayIntersection isNotEqualTo []) then {
                    private _object = (_roadwayIntersection select 0) select 3;

                    if ((_unit getVariable ["KH_var_collectedValidPositionsInterior", []]) isNotEqualTo [netId _object, _object call BIS_fnc_buildingPositions]) then {
                        private _currentValidPositionsInterior = _object call BIS_fnc_buildingPositions;
                        (_data get "ValidPositionsInterior") pushBack [CBA_missionTime, [netId _object, _currentValidPositionsInterior]];
                        _unit setVariable ["KH_var_collectedValidPositionsInterior", [netId _object, _currentValidPositionsInterior]];
                    };

                    private _exits = [];
                    
                    for "_i" from 0 to 999 do {
                        private _exit = _object buildingExit _i;

                        if ((_exit isNotEqualTo []) && (_exit isNotEqualTo [0, 0, 0])) then {
                            _exits pushBack _exit;
                        }
                        else {
                            break;
                        };
                    };

                    if ((_unit getVariable ["KH_var_collectedValidPositionsExterior", []]) isNotEqualTo [netId _object, _exits]) then {
                        (_data get "ValidPositionsExterior") pushBack [CBA_missionTime, [netId _object, _exits]];
                        _unit setVariable ["KH_var_collectedValidPositionsExterior", [netId _object, _exits]];
                    };
                }
                else {
                    if ((_unit getVariable ["KH_var_collectedValidPositionsInterior", []]) isNotEqualTo ["", []]) then {
                        (_data get "ValidPositionsInterior") pushBack [CBA_missionTime, ["", []]];
                        _unit setVariable ["KH_var_collectedValidPositionsInterior", ["", []]];
                    };

                    if ((_unit getVariable ["KH_var_collectedValidPositionsExterior", []]) isNotEqualTo ["", []]) then {
                        (_data get "ValidPositionsExterior") pushBack [CBA_missionTime, ["", []]];
                        _unit setVariable ["KH_var_collectedValidPositionsExterior", ["", []]];
                    };
                };
                
                if ((_unit getVariable ["KH_var_collectedVehicleTurnState", false]) isNotEqualTo (isTurnedOut _unit)) then {
                    private _currentVehicleTurnState = isTurnedOut _unit;
                    (_data get "VehicleTurnState") pushBack [_trueTime, _currentVehicleTurnState];
                    _unit setVariable ["KH_var_collectedVehicleTurnState", _currentVehicleTurnState];
                };
                
                if ((_unit getVariable ["KH_var_collectedVelocity", []]) isNotEqualTo (velocity _unit)) then {
                    private _currentVelocity = velocity _unit;
                    (_data get "Velocity") pushBack [CBA_missionTime, _currentVelocity];
                    _unit setVariable ["KH_var_collectedVelocity", _currentVelocity];
                };

                if ((_unit getVariable ["KH_var_collectedWeaponDirection", []]) isNotEqualTo (_unit weaponDirection (currentWeapon _unit))) then {
                    private _currentWeaponDirection = _unit weaponDirection (currentWeapon _unit);
                    (_data get "WeaponDirection") pushBack [_trueTime, _currentWeaponDirection];
                    _unit setVariable ["KH_var_collectedWeaponDirection", _currentWeaponDirection];
                };

                if ((_unit getVariable ["KH_var_collectedWeaponLowered", false]) isNotEqualTo (weaponLowered _unit)) then {
                    private _currentWeaponLowered = weaponLowered _unit;
                    (_data get "WeaponLowered") pushBack [_trueTime, _currentWeaponLowered];
                    _unit setVariable ["KH_var_collectedWeaponLowered", _currentWeaponLowered];
                };
            },
            true,
            0.1,
            false
        ] call KH_fnc_execute
    ]
];

_unit setVariable ["KH_var_dataCollectionHandlers", _result select 2];
_result;