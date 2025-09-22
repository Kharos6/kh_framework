params [["_unit", objNull, [objNull]]];

createHashMapFromArray [
	["SERVER_MISSION_SESSION_ID", KH_var_serverMissionSessionId],
	["VARIABLE_NAME", [_unit, true] call KH_fnc_getEntityVariableName],
	["GENERATED_VARIABLE_NAME", _unit getVariable ["KH_var_generatedVariableName", false]],
    ["ROLE_DESCRIPTION", roleDescription _unit],
    ["IS_PLAYER", isPlayer _unit],
    ["PLAYER_UID", getPlayerUID _unit],
    ["PLAYER_ID", getPlayerID _unit],
	["CLASS_TYPE", typeOf _unit],
	["AGENT", isAgent (teamMember _unit)],
    ["ENTITY_INFO", call {
        private _entityInfo = getEntityInfo _unit;
        _entityInfo set [4, [_entityInfo select 4, true] call KH_fnc_getEntityVariableName];
        _entityInfo;
    }],
    ["MODEL_INFO", getModelInfo _unit],
    ["BOUNDING_BOX", boundingBoxReal _unit],
    ["BOUNDING_SPHERE_SIZE", sizeOf (typeOf _unit)],
    ["MATERIALS", getObjectMaterials _unit],
    ["TEXTURES", getObjectTextures _unit],
    ["ANIMATION_STATE", animationState _unit],
    ["GESTURE_STATE", gestureState _unit],
    ["MOVES_INFO", getUnitMovesInfo _unit],
    ["SELECTION_NAMES", selectionNames _unit],
    ["ANIM_AIM_PRECISION", getAnimAimPrecision _unit],
    ["ALIVE", alive _unit],
    ["SIMULATION_ENABLED", simulationEnabled _unit],
    ["DYNAMIC_SIMULATION_ENABLED", dynamicSimulationEnabled _unit],
    ["CAN_TRIGGER_DYNAMIC_SIMULATION", canTriggerDynamicSimulation _unit],
    ["HIDDEN", isObjectHidden _unit],
    ["DAMAGE_ALLOWED", isDamageAllowed _unit],
    ["NAME", name _unit],
    ["FACE", face _unit],
    ["SPEAKER", speaker _unit],
    ["PITCH", pitch _unit],
    ["NAME_SOUND", nameSound _unit],
    ["RANK", rank _unit],
    ["UNIT_INSIGNIA", [_unit] call BIS_fnc_getUnitInsignia],
    ["RATING", rating _unit],
    ["UNIT_POS", unitPos _unit],
    ["FORCED_WALK", isForcedWalk _unit],
    ["FORCED_SPEED", getForcedSpeed _unit],
    ["CAPTIVE", captive _unit],
    ["FREEFALL_INFO", getUnitFreefallInfo _unit],
    ["ANIM_SPEED_COEF", getAnimSpeedCoef _unit],
    ["CUSTOM_AIM_COEF", getCustomAimCoef _unit],
    ["SUPPRESSION", getSuppression _unit],
    ["MORALE", morale _unit],
    ["UNIT_LOADOUT", getUnitLoadout _unit],
    ["ALL_UNIT_TRAITS", getAllUnitTraits _unit],
    ["WEAPON_STATE", weaponState _unit],
    ["WEAPON_ZEROING", call {
        private _zeroing = [];

        {
            private _weapon = _x select 2;
            private _muzzle = _x select 3;
            _zeroing pushBack [_weapon, _muzzle, _unit currentZeroing [_weapon, _muzzle]];
        } forEach (_unit weaponsInfo ["", false]);

        _zeroing;			
    }],
    ["WEAPONS_INFO", _unit weaponsInfo ["", false]],
    ["THROWABLES", throwables unit],
    ["FLASHLIGHT_ON", _unit isFlashlightOn (currentWeapon _unit)],
    ["LASER_ON", _unit isLaserOn (currentWeapon _unit)],
    ["IR_LASER_ON", _unit isIRLaserOn (currentWeapon _unit)],
    ["CAN_DEPLOY_WEAPON", canDeployWeapon _unit],
    ["WEAPON_DEPLOYED", isWeaponDeployed _unit],
    ["WEAPON_RESTED", isWeaponRested _unit],
    ["CURRENT_THROWABLE", currentThrowable _unit],
    ["OBJECT_SCALE", getObjectScale _unit],
    ["POS_AGL", _unit modelToWorld [0, 0, 0]],
    ["POS_AGLS", getPos _unit],
    ["POS_ASL", getPosASL _unit],
    ["POS_WORLD", getPosWorld _unit],
    ["POS_ATL", getPosATL _unit],
    ["POS_AGL_VISUAL", _unit modelToWorldVisual [0, 0, 0]],
    ["POS_AGLS_VISUAL", getPosVisual _unit],
    ["POS_ASL_VISUAL", getPosASLVisual _unit],
    ["POS_WORLD_VISUAL", getPosWorldVisual _unit],
    ["POS_ATL_VISUAL", getPosATLVisual _unit],
    ["DIR", getDir _unit],
    ["EYE_POS", eyePos _unit],
    ["EYE_DIR", eyeDirection _unit],
    ["WEAPON_DIR", _unit weaponDirection (currentWeapon _unit)],
    ["VECTOR_DIR", vectorDir _unit],
    ["VECTOR_UP", vectorUp _unit],
    ["VELOCITY", velocity _unit],
    ["VELOCITY_MODEL_SPACE", velocityModelSpace _unit],
    ["INSIDE_BUILDING", insideBuilding _unit],
    ["TOUCHING_GROUND", isTouchingGround _unit],
    ["UNDERWATER_HEAD", ((eyePos _unit) select 2) < 0],
    ["UNDERWATER", underwater _unit],
    ["AIM_POSITION", unitAimPosition _unit],
    ["AIM_POSITION_VISUAL", unitAimPositionVisual _unit],
    ["ATTACHED_TO", attachedTo _unit],
    ["DAMAGE", damage _unit],
    ["ALL_HIT_POINTS_DAMAGE", getAllHitPointsDamage _unit],
    ["BLEEDING_REMAINING", getBleedingRemaining _unit],
    ["OXYGEN_REMAINING", getOxygenRemaining _unit],
    ["LIFE_STATE", lifeState _unit],
    ["INCAPACITATED_STATE", incapacitatedState _unit],
    ["ABLE_TO_BREATHE", isAbleToBreathe _unit],
    ["STANCE", stance _unit],
    ["WEAPON_LOWERED", weaponLowered _unit],
    ["TURNED_OUT", isTurnedOut _unit],
    ["WALKING", isWalking _unit],
    ["SWITCHING_WEAPON", isSwitchingWeapon _unit],
    ["ATTACHED_OBJECTS", call {
        private _attachedObjects = [];
        
        {
            _attachedObjects pushBack ([_x, true] call KH_fnc_getEntityVariableName);
        } forEach (attachedObjects _unit); 
        
        _attachedObjects;
    }],
    ["ATTACHED_TO", [attachedTo _unit, true] call KH_fnc_getEntityVariableName],
    ["COMBAT_MODE", unitCombatMode _unit],
    ["BEHAVIOUR", behaviour _unit],
    ["SKILLS", [
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
    ]],
    ["AI_FEATURES", [
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
    ]],
    ["GROUP_ID", groupId (group _unit)],
    ["ASSIGNED_TEAM", assignedTeam _unit],
    ["VEHICLE_SLOT", call {
        private _vehicleSlot = [_unit] call KH_fnc_getUnitVehicleSlot;
        
        if (_vehicleSlot isNotEqualTo []) then {
            _vehicleSlot set [0, [_vehicleSlot select 0, true] call KH_fnc_getEntityVariableName];
            _vehicleSlot;
        }
        else {
            [];
        };
    }],
    ["COLLISION_DISABLED_WITH", [(collisionDisabledWith _unit) select 0, true] call KH_fnc_getEntityVariableName],
    ["TARGETS", call {
        private _entities = [];

        {
            _x params ["_accuracy", "_target", "_targetSide", "_targetType", "_targetPosition", "_targetAge"];
            _entities pushBack [_accuracy, [_target, true] call KH_fnc_getEntityVariableName, _targetSide, _targetType, _targetPosition, _targetAge];
        } forEach (_unit targetsQuery [objNull, sideUnknown, "", [], 0]);	

        _entities;			
    }],
    ["ALL_VARIABLES", call {
        private _allVariables = [];

        {
            _x params [["_name", "", [""]], "_value", ["_public", false, [true]]];
            _allVariables pushBack [_name, ["", _value] call KH_fnc_serializeValue, _public];
        } forEach (_unit getVariable ["KH_var_persistentVariables", []]);

        _allVariables;
    }]
]