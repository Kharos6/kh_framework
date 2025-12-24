params [["_unit", objNull, [objNull]], ["_attributes", [], [[]]], ["_overrideAttributes", [], [[]]], ["_respectAgent", true, [true]]];
(_attributes select [3]) params ["_type", "_agent", "_savedAttributes"];

if _respectAgent then {
	if _agent then {
		if !(isAgent (teamMember _unit)) then {
			deleteVehicle _unit;
			_unit = createAgent [_type, [0, 0, 0], [], 0, "CAN_COLLIDE"];
		};
	}
	else {
		if (isAgent (teamMember _unit)) then {
			private _group = createGroup [side _unit, true];
			deleteVehicle _unit;
			_unit = _group createUnit [_type, [0, 0, 0], [], 0, "CAN_COLLIDE"];
		};
	};
};

if (_overrideAttributes isNotEqualTo []) then {
	private _currentAttributes = ([_unit] call KH_fnc_getUnitAttributes) select 5;

	{
		if (_x isEqualType 0) then {
			_savedAttributes set [_x, _currentAttributes select _x];
		}
		else {
			_x params [["_index", 0, [0]], "_value"];
			_savedAttributes set [_index, _value];
		};
	} forEach _overrideAttributes;
};

_savedAttributes params [
	"_simulation",
	"_dynamicSimulation",
	"_triggerDynamicSimulation",
	"_hidden",
	"_damageAllowed",
	"_name",
	"_face",
	"_speaker",
	"_pitch",
	"_nameSound",
	"_rank",
	"_insignia",
	"_rating",
	"_unitPos",
	"_forcedWalk",
	"_forcedSpeed",
	"_captive",
	"_unitFreefallInfo",
	"_animSpeedCoef",
	"_customAimCoef",
	"_suppression",
	"_loadout",
	"_traits",
	"_weaponState",
	"_currentZeroing",
	"_currentThrowable",
	"_position",
	"_vectorDir",
	"_vectorUp",
	"_velocityModelSpace",
	"_damage",
	"_hitPointsDamage",
	"_bleedingRemaining",
	"_oxygenRemaining",
	"_lifeState",
	"_combatMode",
	"_behaviour",
	"_skill",
	"_aiFeatures",
	"_group",
	"_assignedTeam",
	"_vehicle",
	"_collisionDisabledWith",
	"_physicsCollisionFlag",
	"_targets",
	"_variables"
];

[
	[
        _unit,
		_simulation,
        _hidden,
        _rank,
		_insignia,
        _loadout,
        _position,
		_group,
		_assignedTeam
	], 
	{
		params [
			"_unit",
			"_simulation",
			"_hidden",
			"_rank",
			"_insignia",
			"_loadout",
			"_position",
			"_group",
			"_assignedTeam"
		];

		_unit enableSimulationGlobal _simulation;
		_unit hideObjectGlobal _hidden;
		_unit setUnitRank _rank;
		[_unit, _insignia] call BIS_fnc_setUnitInsignia;
		_unit setUnitLoadout _loadout;
		_unit setPosATL _position;
		_group = call {
			private _allGroups = groups (side (group _unit));
			private _index = _allGroups findIf {(_group isEqualTo (groupId _x));};

			if (_index isNotEqualTo -1) then {
				_allGroups select _index;
			}
			else {
				grpNull;
			};
		};

		if !(isNull _group) then {
			[_unit] joinSilent _group;
		};
		
		_assignedTeam = toUpperANSI _assignedTeam;

		if ((_assignedTeam isEqualTo "MAIN") || (_assignedTeam isEqualTo "RED") || (_assignedTeam isEqualTo "GREEN") || (_assignedTeam isEqualTo "BLUE") || (_assignedTeam isEqualTo "YELLOW")) then {
			_unit assignTeam _assignedTeam;
		};
	}, 
	"SERVER", 
	true, 
	false
] call KH_fnc_execute;

[
	[
		_unit,
		_damageAllowed,
		_rating,
		_unitPos,
		_forcedWalk,
		_forcedSpeed,
		_captive,
		_unitFreefallInfo,
		_suppression,
		_traits,
		_weaponState,
		_currentZeroing,
		_currentThrowable,
		_vectorDir,
		_vectorUp,
		_velocityModelSpace,
		_damage,
		_hitPointsDamage,
		_bleedingRemaining,
		_oxygenRemaining,
		_lifeState,
		_combatMode,
		_behaviour,
		_skill,
		_aiFeatures,
		_vehicle,
		_collisionDisabledWith,
		_targets,
		_variables
	], 
	{
		params [
			"_unit",
			"_damageAllowed",
			"_rating",
			"_unitPos",
			"_forcedWalk",
			"_forcedSpeed",
			"_captive",
			"_unitFreefallInfo",
			"_suppression",
			"_traits",
			"_weaponState",
			"_currentZeroing",
			"_currentThrowable",
			"_vectorDir",
			"_vectorUp",
			"_velocityModelSpace",
			"_damage",
			"_hitPointsDamage",
			"_bleedingRemaining",
			"_oxygenRemaining",
			"_lifeState",
			"_combatMode",
			"_behaviour",
			"_skill",
			"_aiFeatures",
			"_vehicle",
			"_collisionDisabledWith",
			"_targets",
			"_variables"
		];

		_unit allowDamage _damageAllowed;
		[_unit, _rating] call KH_fnc_setRating;
		_unit setUnitPos _unitPos;
		_unit forceWalk _forcedWalk;
		_unit forceSpeed _forcedSpeed;
		_unit setCaptive _captive;
		_unit setUnitFreefallHeight (_unitFreefallInfo select 2);
		_unit setSuppression _suppression;
		
		{
			_X params ["_name", "_value"];
			_unit setUnitTrait [_name, _value];
		} forEach _traits;

		_weaponState params ["_weapon", "_muzzle", "_firemode"];
		_unit selectWeapon [_weapon, _muzzle, _fireMode];

		{
			_unit setWeaponZeroing _x;
		} forEach _currentZeroing;

		_unit selectThrowable (_currentThrowable select 1);
		_unit setVectorDirAndUp [_vectorDir, _vectorUp];
		_unit setVelocityModelSpace _velocityModelSpace;
		_unit setDamage _damage;
		private _hitPointNames = _hitPointsDamage select 0;
		private _hitPointValues = _hitPointsDamage select 2;

		for "_i" from 0 to ((count _hitPointNames) - 1) do {
			_unit setHitPointDamage [_hitPointNames select _i, _hitPointValues select _i];
		};

		_unit setBleedingRemaining _bleedingRemaining;
		_unit setOxygenRemaining _oxygenRemaining;

		if ((_lifeState isEqualTo "INCAPACITATED") || (_lifeState isEqualTo "UNCONSCIOUS") || (_unit getVariable ["KH_var_incapacitated", false])) then {
			_unit setUnconscious true;
		};

		_unit setUnitCombatMode _combatMode;
		_unit setBehaviourStrong _behaviour;
		_unit setSkill (_skill select 0);
		_unit setSkill ["aimingAccuracy", _skill select 1];
		_unit setSkill ["aimingShake", _skill select 2];
		_unit setSkill ["aimingSpeed", _skill select 3];
		_unit setSkill ["spotDistance", _skill select 4];
		_unit setSkill ["spotTime", _skill select 5];
		_unit setSkill ["courage", _skill select 6];
		_unit setSkill ["reloadSpeed", _skill select 7];
		_unit setSkill ["commanding", _skill select 8];
		_unit setSkill ["general", _skill select 9];
		_unit enableAIFeature ["AUTOTARGET", _aiFeatures select 0];
		_unit enableAIFeature ["MOVE", _aiFeatures select 1];
		_unit enableAIFeature ["TARGET", _aiFeatures select 2];
		_unit enableAIFeature ["TEAMSWITCH", _aiFeatures select 3];
		_unit enableAIFeature ["WEAPONAIM", _aiFeatures select 4];
		_unit enableAIFeature ["ANIM", _aiFeatures select 5];
		_unit enableAIFeature ["FSM", _aiFeatures select 6];
		_unit enableAIFeature ["AIMINGERROR", _aiFeatures select 7];
		_unit enableAIFeature ["SUPPRESSION", _aiFeatures select 8];
		_unit enableAIFeature ["CHECKVISIBLE", _aiFeatures select 9];
		_unit enableAIFeature ["AUTOCOMBAT", _aiFeatures select 10];
		_unit enableAIFeature ["COVER", _aiFeatures select 11];
		_unit enableAIFeature ["PATH", _aiFeatures select 12];
		_unit enableAIFeature ["MINEDETECTION", _aiFeatures select 13];
		_unit enableAIFeature ["LIGHTS", _aiFeatures select 14];
		_unit enableAIFeature ["NVG", _aiFeatures select 15];
		_unit enableAIFeature ["RADIOPROTOCOL", _aiFeatures select 16];
		_unit enableAIFeature ["FIREWEAPON", _aiFeatures select 17];

		if (_vehicle isNotEqualTo []) then {
			_vehicle set [0, ["OBJECT", _vehicle] call KH_fnc_getEntityByIdentifier];
			[_unit, _vehicle] call KH_fnc_setUnitVehicleSlot;
		};

		private _collisionEntity = ["OBJECT", _collisionDisabledWith] call KH_fnc_getEntityByIdentifier;

		if !(isNull _collisionEntity) then {
			_unit disableCollisionWith _collisionEntity;
		};

		{
			_x params ["_accuracy", "_target"];
			_target = ["OBJECT", _target] call KH_fnc_getEntityByIdentifier;

			if !(isNull _target) then {
				_unit reveal [_target, _accuracy];
			};
		} forEach _targets;

		{
			_unit setVariable _x;
		} forEach _variables;
	}, 
	_unit, 
	true, 
	false
] call KH_fnc_execute;

[
	[
		_unit,
		_dynamicSimulation,
		_triggerDynamicSimulation,
		_name,
		_face,
		_speaker,
		_pitch,
		_nameSound,
		_animSpeedCoef,
		_customAimCoef,
		_physicsCollisionFlag
	], 
	{
		params [
			"_unit",
			"_dynamicSimulation",
			"_triggerDynamicSimulation",
			"_name",
			"_face",
			"_speaker",
			"_pitch",
			"_nameSound",
			"_animSpeedCoef",
			"_customAimCoef",
			"_physicsCollisionFlag"
		];

		_unit enableDynamicSimulation _dynamicSimulation;
		_unit triggerDynamicSimulation _triggerDynamicSimulation;
		_unit setName _name;
		_unit setFace _face;
		_unit setSpeaker _speaker;
		_unit setPitch _pitch;
		_unit setNameSound _nameSound;
		_unit setAnimSpeedCoef _animSpeedCoef;
		_unit setCustomAimCoef _customAimCoef;
		_unit setPhysicsCollisionFlag _physicsCollisionFlag;
	}, 
	"GLOBAL", 
	true, 
	["JIP", _unit, false, ["KH_var_savedUnitAttributes_", [_unit, true] call KH_fnc_getEntityVariableName] joinString ""]
] call KH_fnc_execute;

_unit;