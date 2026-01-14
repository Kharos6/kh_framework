params [["_object", objNull, [objNull]]];
private _name = ["KH_var_collectedData_", CBA_missionTime] joinString "";
_object setVariable [_name, createHashMap];
private _data = _object getVariable _name;

if !(_object getVariable ["KH_var_allowDataCollection", true]) exitWith {
    [
        _object getVariable ["KH_var_allCollectedData", []],
        _data,
        []
    ];
};

if ((_object getVariable ["KH_var_dataCollectionHandlers", []]) isNotEqualTo []) then {
    [_object getVariable ["KH_var_dataCollectionHandlers", []]] call KH_fnc_removeHandler;
    _object setVariable ["KH_var_dataCollectionHandlers", []];
};

if (_object isNil "KH_var_allCollectedData") then {
    _object setVariable ["KH_var_allCollectedData", []];
};

(_object getVariable ["KH_var_allCollectedData", []]) pushBack [CBA_missionTime, _data];
_data set ["AmmoCargo", []];
_data set ["AngularVelocity", []];
_data set ["CargoLoaded", []];
_data set ["CargoUnloaded", []];
_data set ["CollisionLights", []];
_data set ["Damage", []];
_data set ["Direction", []];
_data set ["Engine", []];
_data set ["Fuel", []];
_data set ["FuelCargo", []];
_data set ["Gear", []];

_data set [
    "Header", 
    [
        netId _object,
        [_object] call KH_fnc_getObjectAttributes
    ]
];

_data set ["Hit", []];
_data set ["IncomingMissile", []];
_data set ["Killed", -1];
_data set ["Lights", []];
_data set ["Locked", []];
_data set ["LockedInventory", []];
_data set ["Position", []];
_data set ["PylonChange", []];
_data set ["Reload", []];
_data set ["RepairCargo", []];
_data set ["Velocity", []];
_data set ["WaterFill", []];

private _result = [
    _object getVariable ["KH_var_allCollectedData", []],
    _data,
    [
        [
            ["ENTITY", _object, "REMOTE"],
            "CargoLoaded",
            [_object, _data],
            {
                params ["_parentVehicle", "_cargoVehicle"];
                _args params ["_object", "_data"];
                (_data get "CargoLoaded") pushBack [(CBA_missionTime - ([_object, false] call KH_fnc_getLatency)) max 0, [netId _parentVehicle, typeOf _parentVehicle, netId _cargoVehicle, typeOf _cargoVehicle]];
            }
        ] call KH_fnc_addEventHandler,
        [
            ["ENTITY", _object, "REMOTE"],
            "CargoUnloaded",
            [_object, _data],
            {
                params ["_parentVehicle", "_cargoVehicle"];
                _args params ["_object", "_data"];
                (_data get "CargoUnloaded") pushBack [(CBA_missionTime - ([_object, false] call KH_fnc_getLatency)) max 0, [netId _parentVehicle, typeOf _parentVehicle, netId _cargoVehicle, typeOf _cargoVehicle]];
            }
        ] call KH_fnc_addEventHandler,
        [
            ["ENTITY", _object, "REMOTE"],
            "Engine",
            [_data],
            {
                params ["_object", "_engineState"];
                _args params ["_data"];
                (_data get "Engine") pushBack [(CBA_missionTime - ([_object, false] call KH_fnc_getLatency)) max 0, _engineState];
            }
        ] call KH_fnc_addEventHandler,
        [
            ["ENTITY", _object, "REMOTE"],
            "Gear",
            [_data],
            {
                params ["_object", "_gearState"];
                _args params ["_data"];
                (_data get "Gear") pushBack [(CBA_missionTime - ([_object, false] call KH_fnc_getLatency)) max 0, _gearState];
            }
        ] call KH_fnc_addEventHandler,
        [
            ["ENTITY", _object, "REMOTE"],
            "HandleDamage",
            [_data],
            {
                private _object = param [0];
                private _source = param [3];
                private _projectile = param [4];
                private _instigator = param [6];
                private _hitPoint = param [7];
                _args params ["_data"];
                (_data get "Hit") pushBack [(CBA_missionTime - ([_object, false] call KH_fnc_getLatency)) max 0, [netId _source, typeOf _source, _projectile, netId _instigator, _hitPoint]];
            }
        ] call KH_fnc_addEventHandler,
        [
            ["ENTITY", _object, "REMOTE"],
            "IncomingMissile",
            [_data],
            {
                params ["_object", "_ammo", "_shooter", "_instigator", "_missile"];
                _args params ["_data"];
                (_data get "IncomingMissile") pushBack [(CBA_missionTime - ([_object, false] call KH_fnc_getLatency)) max 0, [_ammo, netId _shooter, typeOf _shooter, netId _instigator, netId _missile]];
            }
        ] call KH_fnc_addEventHandler,
        [
            ["ENTITY", _object, "REMOTE"],
            "Killed",
            [_data],
            {
                params ["_object"];
                _args params ["_data"];
                _data set ["Killed", (CBA_missionTime - ([_object, false] call KH_fnc_getLatency)) max 0];
            }
        ] call KH_fnc_addEventHandler,
        [
            ["ENTITY", _object, "REMOTE"],
            "PylonChanged",
            [_data],
            {
                params ["_object", "_pylonIndex", "_oldMagazine", "_newMagazine"];
                _args params ["_data"];
                (_data get "PylonChange") pushBack [(CBA_missionTime - ([_object, false] call KH_fnc_getLatency)) max 0, [_pylonIndex, _oldMagazine, _newMagazine]];
            }
        ] call KH_fnc_addEventHandler,
        [
            ["ENTITY", _object, "REMOTE"],
            "Reloaded",
            [_data],
            {
                params ["_object", "_weapon", "_muzzle", "_newMagazine"];
                _args params ["_data"];
                (_data get "Reload") pushBack [(CBA_missionTime - ([_object, false] call KH_fnc_getLatency)) max 0, [_weapon, _muzzle, _newMagazine]];
            }
        ] call KH_fnc_addEventHandler,
        [
            [_object, _data],
            {
                params ["_object", "_data"];
                private _trueTime = (CBA_missionTime - ([_object, false] call KH_fnc_getLatency)) max 0;

                if ((_object getVariable ["KH_var_collectedAmmoCargo", -1]) isNotEqualTo (getAmmoCargo _object)) then {
                    private _currentAmmoCargo = getAmmoCargo _object;
                    (_data get "AmmoCargo") pushBack [_trueTime, _currentAmmoCargo];
                    _object setVariable ["KH_var_collectedAmmoCargo", _currentAmmoCargo];
                };

                if ((_object getVariable ["KH_var_collectedAngularVelocity", []]) isNotEqualTo (angularVelocity _object)) then {
                    private _currentAngularVelocity = angularVelocity _object;
                    (_data get "AngularVelocity") pushBack [CBA_missionTime, _currentAngularVelocity];
                    _object setVariable ["KH_var_collectedAngularVelocity", _currentAngularVelocity];
                };

                if ((_object getVariable ["KH_var_collectedCollisionLights", false]) isNotEqualTo (isCollisionLightOn _object)) then {
                    private _currentCollisionLights = isCollisionLightOn _object;
                    (_data get "CollisionLights") pushBack [_trueTime, _currentCollisionLights];
                    _object setVariable ["KH_var_collectedCollisionLights", _currentCollisionLights];
                };

                private _hitPointsDamage = getAllHitPointsDamage _object;
                private _processedHitPointsDamage = [];

                {
                    _processedHitPointsDamage pushBack [_x, (_hitPointsDamage select 2) select _forEachIndex];
                } forEach (_hitPointsDamage select 0);

                if ((_object getVariable ["KH_var_collectedDamage", []]) isNotEqualTo [damage _object, _processedHitPointsDamage]) then {
                    private _currentDamage = [damage _object, _processedHitPointsDamage];
                    (_data get "Damage") pushBack [_trueTime, _currentDamage];
                    _object setVariable ["KH_var_collectedDamage", _currentDamage];
                };

                if ((_object getVariable ["KH_var_collectedDirection", []]) isNotEqualTo [vectorDirVisual _object, vectorUpVisual _object]) then {
                    private _currentDirection = [vectorDirVisual _object, vectorUpVisual _object];
                    (_data get "Direction") pushBack [CBA_missionTime, _currentDirection];
                    _object setVariable ["KH_var_collectedDirection", _currentDirection];
                };

                if ((_object getVariable ["KH_var_collectedFuel", -1]) isNotEqualTo (fuel _object)) then {
                    private _currentFuel = fuel _object;
                    (_data get "Fuel") pushBack [_trueTime, _currentFuel];
                    _object setVariable ["KH_var_collectedFuel", _currentFuel];
                };

                if ((_object getVariable ["KH_var_collectedFuelCargo", -1]) isNotEqualTo (getFuelCargo _object)) then {
                    private _currentFuelCargo = getFuelCargo _object;
                    (_data get "FuelCargo") pushBack [_trueTime, _currentFuelCargo];
                    _object setVariable ["KH_var_collectedFuelCargo", _currentFuelCargo];
                };

                if ((_object getVariable ["KH_var_collectedLights", false]) isNotEqualTo (isLightOn _object)) then {
                    private _currentLights = isLightOn _object;
                    (_data get "Lights") pushBack [_trueTime, _currentLights];
                    _object setVariable ["KH_var_collectedLights", _currentLights];
                };

                if ((_object getVariable ["KH_var_collectedLocked", false]) isNotEqualTo (locked _object)) then {
                    private _currentLocked = locked _object;
                    (_data get "Locked") pushBack [_trueTime, _currentLocked];
                    _object setVariable ["KH_var_collectedLocked", _currentLocked];
                };

                if ((_object getVariable ["KH_var_collectedLockedInventory", false]) isNotEqualTo (lockedInventory _object)) then {
                    private _currentLockedInventory = lockedInventory _object;
                    (_data get "LockedInventory") pushBack [_trueTime, _currentLockedInventory];
                    _object setVariable ["KH_var_collectedLockedInventory", _currentLockedInventory];
                };

                if ((_object getVariable ["KH_var_collectedPosition", []]) isNotEqualTo ([getPosATLVisual _object, getPosASLVisual _object, getPosVisual _object])) then {
                    private _currentPosition = [getPosATLVisual _object, getPosASLVisual _object, getPosVisual _object];
                    (_data get "Position") pushBack [CBA_missionTime, _currentPosition];
                    _object setVariable ["KH_var_collectedPosition", _currentPosition];
                };
                
                if ((_object getVariable ["KH_var_collectedRepairCargo", -1]) isNotEqualTo (getRepairCargo _object)) then {
                    private _currentRepairCargo = getRepairCargo _object;
                    (_data get "RepairCargo") pushBack [_trueTime, _currentRepairCargo];
                    _object setVariable ["KH_var_collectedRepairCargo", _currentRepairCargo];
                };

                if ((_object getVariable ["KH_var_collectedVelocity", []]) isNotEqualTo (velocity _object)) then {
                    private _currentVelocity = velocity _object;
                    (_data get "Velocity") pushBack [CBA_missionTime, _currentVelocity];
                    _object setVariable ["KH_var_collectedVelocity", _currentVelocity];
                };

                if ((_object getVariable ["KH_var_collectedWaterFill", -1]) isNotEqualTo (getWaterFillPercentage _object)) then {
                    private _currentWaterFill = getWaterFillPercentage _object;
                    (_data get "WaterFill") pushBack [_trueTime, _currentWaterFill];
                    _object setVariable ["KH_var_collectedWaterFill", _currentWaterFill];
                };
            },
            true,
            0.1,
            false
        ] call KH_fnc_execute
    ]
];

_object setVariable ["KH_var_dataCollectionHandlers", _result select 2];
_result;