params ["_unit", "_attributes", ["_group", true], ["_variableName", false], ["_identity", true], ["_loadout", true], ["_position", true], ["_rotation", true], ["_vehicle", true], ["_behavior", true], ["_skill", true], ["_AI", true], ["_unitTraits", true]];

if _group then {
	private _savedGroup = _attributes select 1;

	if !(isNull _savedGroup) then {
		[_unit] joinSilent _savedGroup;
	};
};

if _variableName then {
	if ((_attributes select 3) != "") then {
		[
			[_unit, _attributes],
			{
				params ["_unit", "_attributes"];
				_unit setVehicleVarName (_attributes select 3);
			},
			["JIP", "GLOBAL", _unit, false, false],
			"THIS_FRAME"
		] call KH_fnc_execute;
	};
};

if _identity then {
	[
		[_unit, _attributes],
		{
			params ["_unit", "_attributes"];
			_unit setIdentity (_attributes select 4);
			_unit setFace (_attributes select 5);
			_unit setSpeaker (_attributes select 6);
		},
		["JIP", "GLOBAL", _unit, false, false],
		"THIS_FRAME"
	] call KH_fnc_execute;
};

if _loadout then {
	_unit setUnitLoadout (_attributes select 7);
};

if _position then {
	_unit setPosATL (_attributes select 8);
};

if _rotation then {
	[_unit, _attributes select 9, false] call KH_fnc_setRotation;
};

if _vehicle then {
	private _savedVehicle = _attributes select 10;

	if !(isNull _savedVehicle) then {
		_unit moveInAny _savedVehicle;
	};
};

if _behaviour then {
	[
		[_unit, _attributes],
		{
			params ["_unit", "_attributes"];
			(group _unit) setBehaviourStrong (_attributes select 11);
		},
		_unit,
		"THIS_FRAME"
	] call KH_fnc_execute;
};

if _skill then {
	[
		[_unit, _attributes],
		{
			params ["_unit", "_attributes"];
			_unit setSkill (_attributes select 12);
			private _skills = _attributes select 13;
			_unit setSkill ["aimingAccuracy", _skills select 0];
			_unit setSkill ["aimingShake", _skills select 1];
			_unit setSkill ["aimingSpeed", _skills select 2];
			_unit setSkill ["spotDistance", _skills select 3];
			_unit setSkill ["spotTime", _skills select 4];
			_unit setSkill ["courage", _skills select 5];
			_unit setSkill ["reloadSpeed", _skills select 6];
			_unit setSkill ["commanding", _skills select 7];
			_unit setSkill ["general", _skills select 8];
		},
		_unit,
		"THIS_FRAME"
	] call KH_fnc_execute;
};

if _AI then {
	[
		[_unit, _attributes],
		{
			params ["_unit", "_attributes"];
			private _features = _attributes select 14;
			_unit enableAIFeature ["AUTOTARGET", _features select 0];
			_unit enableAIFeature ["MOVE", _features select 1];
			_unit enableAIFeature ["TARGET", _features select 2];
			_unit enableAIFeature ["TEAMSWITCH", _features select 3];
			_unit enableAIFeature ["WEAPONAIM", _features select 4];
			_unit enableAIFeature ["ANIM", _features select 5];
			_unit enableAIFeature ["FSM", _features select 6];
			_unit enableAIFeature ["AIMINGERROR", _features select 7];
			_unit enableAIFeature ["SUPPRESSION", _features select 8];
			_unit enableAIFeature ["CHECKVISIBLE", _features select 9];
			_unit enableAIFeature ["AUTOCOMBAT", _features select 10];
			_unit enableAIFeature ["COVER", _features select 11];
			_unit enableAIFeature ["PATH", _features select 12];
			_unit enableAIFeature ["MINEDETECTION", _features select 13];
			_unit enableAIFeature ["LIGHTS", _features select 14];
			_unit enableAIFeature ["NVG", _features select 15];
			_unit enableAIFeature ["RADIOPROTOCOL", _features select 16];
			_unit enableAIFeature ["FIREWEAPON", _features select 17];
		},
		_unit,
		"THIS_FRAME"
	] call KH_fnc_execute;
};

if _unitTraits then {
	[
		[_unit, _attributes],
		{
			params ["_unit", "_attributes"];
			private _traits = _attributes select 15;
			private _traitCount = (count _traits) -1;
			private _i = 0;

			for "_i" from 0 to _traitCount do {	
				_unit setUnitTrait [(_traits select _i) select 0, (_traits select _i) select 1];
			};
		},
		_unit,
		"THIS_FRAME"
	] call KH_fnc_execute;
};

true;