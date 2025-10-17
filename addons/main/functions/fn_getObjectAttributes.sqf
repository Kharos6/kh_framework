params [["_object", objNull, [objNull]]];

[
	KH_var_serverMissionSessionId,
	[_object, true] call KH_fnc_getEntityVariableName,
    _object getVariable ["KH_var_generatedVariableName", false],
	typeOf _object,
    isSimpleObject _object,
	[
		simulationEnabled _object,
		dynamicSimulationEnabled _object,
		isObjectHidden _object,
		isDamageAllowed _object,
		getPosATL _object,
		vectorDir _object,
		vectorUp _object,
		velocityModelSpace _object,
        angularVelocityModelSpace _object,
		damage _object,
		getAllHitPointsDamage _object,
        locked _object,
        lockedInventory _object,
        [_object] call KH_fnc_getObjectInventory,
        isEngineOn _object,
        isLightOn _object,
        isCollisionLightOn _object,
        allowedService _object,
        equipmentDisabled _object,
        [_object] call KH_fnc_getVehicleLoadout,
        getFuelConsumptionCoef _object,
        getWaterLeakiness _object,
        getWaterFillPercentage _object,
        isVehicleCargo _object,
		[(collisionDisabledWith _object) select 0, true] call KH_fnc_getEntityVariableName,
		call {
			private _allVariables = [];

			{
				_allVariables pushBack _x;
			} forEach (_object getVariable ["KH_var_persistentVariables", []]);

			_allVariables;
		}
	]
];