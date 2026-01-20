params [["_unit", objNull, [objNull]]];

[
	KH_var_serverMissionSessionId,
	[_unit, true] call KH_fnc_getEntityVariableName,
	_unit getVariable ["KH_var_generatedVariableName", false],
	typeOf _unit,
	isAgent (teamMember _unit),
	[
		simulationEnabled _unit,
		dynamicSimulationEnabled _unit,
		canTriggerDynamicSimulation _unit,
		isObjectHidden _unit,
		isDamageAllowed _unit,
		name _unit,
		face _unit,
		speaker _unit,
		pitch _unit,
		nameSound _unit,
		rank _unit,
		[_unit] call BIS_fnc_getUnitInsignia,
		rating _unit,
		unitPos _unit,
		isForcedWalk _unit,
		getForcedSpeed _unit,
		captive _unit,
		getUnitFreefallInfo _unit,
		getAnimSpeedCoef _unit,
		getCustomAimCoef _unit,
		getSuppression _unit,
		getUnitLoadout _unit,
		getAllUnitTraits _unit,
		weaponState _unit,
		(_unit weaponsInfo ["", false]) apply {
			private _weapon = _x select 2;
			private _muzzle = _x select 3;
			[_weapon, _muzzle, (_unit currentZeroing [_weapon, _muzzle]) select 1];
		},
		currentThrowable _unit,
		getPosATL _unit,
		vectorDir _unit,
		vectorUp _unit,
		velocityModelSpace _unit,
		damage _unit,
		getAllHitPointsDamage _unit,
		getBleedingRemaining _unit,
		getOxygenRemaining _unit,
		lifeState _unit,
		unitCombatMode _unit,
		behaviour _unit,
		[
			skill _unit,
			_unit skill "aimingAccuracy",
			_unit skill "aimingShake",
			_unit skill "aimingSpeed",
			_unit skill "spotDistance",
			_unit skill "spotTime",
			_unit skill "courage",
			_unit skill "reloadSpeed",
			_unit skill "commanding",
			_unit skill "general"
		],
		[
			_unit checkAIFeature "AUTOTARGET",
			_unit checkAIFeature "MOVE",
			_unit checkAIFeature "TARGET",
			_unit checkAIFeature "TEAMSWITCH",
			_unit checkAIFeature "WEAPONAIM",
			_unit checkAIFeature "ANIM",
			_unit checkAIFeature "FSM",
			_unit checkAIFeature "AIMINGERROR",
			_unit checkAIFeature "SUPPRESSION",
			_unit checkAIFeature "CHECKVISIBLE",
			_unit checkAIFeature "AUTOCOMBAT",
			_unit checkAIFeature "COVER",
			_unit checkAIFeature "PATH",
			_unit checkAIFeature "MINEDETECTION",
			_unit checkAIFeature "LIGHTS",
			_unit checkAIFeature "NVG",
			_unit checkAIFeature "RADIOPROTOCOL",
			_unit checkAIFeature "FIREWEAPON"
		],
		groupId (group _unit),
		assignedTeam _unit,
		call {
			private _vehicleSlot = [_unit] call KH_fnc_getUnitVehicleSlot;
			
			if (_vehicleSlot isNotEqualTo []) then {
				_vehicleSlot set [0, [_vehicleSlot select 0, true] call KH_fnc_getEntityVariableName];
				_vehicleSlot;
			}
			else {
				[];
			};
		},
		[(collisionDisabledWith _unit) select 0, true] call KH_fnc_getEntityVariableName,
		(getPhysicsCollisionFlag _unit) select 0,
		(_unit targetsQuery [objNull, sideUnknown, "", [], 0]) apply {
			_x params ["_accuracy", "_target", "_targetSide", "_targetType", "_targetPosition", "_targetAge"];
			[_accuracy, [_target, true] call KH_fnc_getEntityVariableName, _targetSide, _targetType, _targetPosition, _targetAge];
		},
		_unit getVariable ["KH_var_persistentVariables", []]
	]
];