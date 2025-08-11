params ["_unit", "_animation", "_position", "_rotation", "_target", "_positionReset", "_rotationReset", "_duration", ["_camera", false], ["_disableUserInput", true], ["_disableDamage", true], ["_setCaptive", false], ["_interruptType", "UNIT"], ["_event", "KH_eve_scriptedAnimationFinished"]];

if (_duration != -1) then {
	private _event = call KH_fnc_generateUid;
	private _animationId = [missionNamespace, "KH_var_setAnimationId", false, false] call KH_fnc_atomicVariable;
	private _targetHitHandler = [];
	private _unitHitHandler = [];
	private _targetGroupHitHandler = [];
	private _unitGroupHitHandler = [];
	private _resetPosition = getPosATL _positionReset;
	private _resetRotation = vectorDir _rotationReset;
	private _cameraObject = objNull;
	private _cameraTarget = objNull;
	private _attachObject = KH_var_logicGroup createUnit ["Logic", [0, 0, 0], [], 0, "CAN_COLLIDE"];
	[_attachObject, [_position, "ATL", false], [_rotation, false]] call KH_fnc_setTransforms;

	if (isPlayer _unit) then {
		if (_interruptType == "GROUP") then {
			_interruptType = "UNIT";
		};

		[[[], true], "KH_fnc_closeInterface", _unit, true] call KH_fnc_execute;
		
		if _disableUserInput then {
			[[false, false], "KH_fnc_toggleUserInput", _unit, true] call KH_fnc_execute;
		};

		if _camera then {
			_cameraObject = createVehicle ["KH_HelperSquare", _x, [], 0, "CAN_COLLIDE"];
			_cameraObject attachTo [_unit, [-0.12, 0, 0.15], "Head", true];	
			_cameraTarget = createVehicle ["KH_HelperSquare", _x, [], 0, "CAN_COLLIDE"];
			_cameraTarget attachTo [_unit, [0, 15, 0], "Head", true];
			[[[_cameraObject], [_cameraTarget], [0.75], [0], [_duration], [-1], false, false], "KH_fnc_cameraSequence", _unit, true] call KH_fnc_execute;
		};
	};

	switch _interruptType do {
		case "GROUP": {
			_targetGroupHitHandler = [
				["STANDARD", group _target, "LOCAL"],
				"CombatModeChanged",
				[_event],
				{
					_args params ["_event"];
					private _newMode = _this select 1;

					if ((_newMode == "AWARE") || (_newMode == "COMBAT")) then {
						[_event, []] call CBA_fnc_serverEvent;
					};
				}
			] call KH_fnc_addEventHandler;

			_unitGroupHitHandler = [
				["STANDARD", group _unit, "LOCAL"],
				"CombatModeChanged",
				[_event],
				{
					_args params ["_event"];
					private _newMode = _this select 1;

					if ((_newMode == "AWARE") || (_newMode == "COMBAT")) then {
						[_event, []] call CBA_fnc_serverEvent;
					};
				}
			] call KH_fnc_addEventHandler;
		};

		case "UNIT": {
			_targetHitHandler = [
				["STANDARD", _target, "LOCAL"],
				"Dammaged",
				[_event],
				{
					_args params ["_event"];
					[_event, []] call CBA_fnc_serverEvent;
				}
			] call KH_fnc_addEventHandler;

			_unitHitHandler = [
				["STANDARD", _unit, "LOCAL"],
				"Dammaged",
				[_event],
				{
					_args params ["_event"];
					[_event, []] call CBA_fnc_serverEvent;
				}
			] call KH_fnc_addEventHandler;
		};

		case "NONE": {
			if !(isPlayer _unit) then {
				[
					[_unit],
					{
						params ["_unit"];
						_unit disableAI "ALL";
					},
					_unit,
					true
				] call KH_fnc_execute;
			};
		};
	};

	if _disableDamage then {
		[
			[_unit],
			{
				params ["_unit"];
				_unit allowDamage false;
			},
			_unit,
			true
		] call KH_fnc_execute;
	};

	if _setCaptive then {
		[
			[_unit],
			{
				params ["_unit"];
				_unit setCaptive true;
			},
			_unit,
			true
		] call KH_fnc_execute;
	};

	_unit attachTo [_attachObject, [0, 0, 0]];
	[_unit, _rotation, false] call KH_fnc_setRotation;

	[
		[_unit, _animation],
		{
			params ["_unit", "_animation"];
			_unit switchMove _animation;
		},
		"GLOBAL",
		true
	] call KH_fnc_execute;
	
	private _interruptCheck = [];

	if (_interruptType != "NONE") then {
		_interruptCheck = [
			"CBA",
			_event,
			[_unit, _target, _camera, _disableUserInput, _disableDamage, _setCaptive, _interruptType, _event, _resetPosition, _resetRotation, _attachObject, _cameraObject, _cameraTarget, _targetHitHandler, _unitHitHandler, _targetGroupHitHandler, _unitGroupHitHandler, _animationId],
			{
				_args params ["_unit", "_target", "_camera", "_disableUserInput", "_disableDamage", "_setCaptive", "_interruptType", "_event", "_resetPosition", "_resetRotation", "_attachObject", "_cameraObject", "_cameraTarget", "_targetHitHandler", "_unitHitHandler", "_targetGroupHitHandler", "_unitGroupHitHandler", "_animationId"];
				missionNamespace setVariable [_animationId, true];
				[_event, [_unit, _target, false]] call CBA_fnc_globalEvent;

				if _disableDamage then {
					[
						[_unit],
						{
							params ["_unit"];
							_unit allowDamage true;
						},
						_unit,
						true
					] call KH_fnc_execute;
				};
				
				if ((alive _unit) && _setCaptive) then {
					[
						[_unit],
						{
							params ["_unit"];
							_unit setCaptive false;
						},
						_unit,
						true
					] call KH_fnc_execute;
				};
				
				if ((isPlayer _unit) && _disableUserInput) then {
					[[true, false], "KH_fnc_toggleUserInput", _unit, true] call KH_fnc_execute;
				};
				
				detach _unit;

				[
					[_unit],
					{
						params ["_unit"];
						_unit switchMove [""];
					},
					"GLOBAL",
					true
				] call KH_fnc_execute;

				[_unit, [_resetPosition, "ATL", false], [_resetRotation, false]] call KH_fnc_setTransforms;
				deleteVehicle _attachObject;
				
				if _camera then {
					deleteVehicle _cameraObject;
					deleteVehicle _cameraTarget;
				};
				
				if (!(isPlayer _unit) && (alive _unit) && (_interruptType == "NONE")) then {
					[
						[_unit],
						{
							params ["_unit"];
							_unit enableAI "ALL";
						},
						_unit,
						true
					] call KH_fnc_execute;
				};
				
				if (_targetHitHandler isNotEqualTo []) then {
					[_targetHitHandler] call KH_fnc_removeEventHandler;
				};
			
				if (_unitHitHandler isNotEqualTo []) then {
					[_unitHitHandler] call KH_fnc_removeEventHandler;
				};

				if (_targetGroupHitHandler isNotEqualTo []) then {
					[_targetGroupHitHandler] call KH_fnc_removeEventHandler;
				};
			
				if (_unitGroupHitHandler isNotEqualTo []) then {
					[_unitGroupHitHandler] call KH_fnc_removeEventHandler;
				};
				
				[_localId] call KH_fnc_removeEventHandler;
			}
		] call KH_fnc_addEventHandler;
	};

	if (_duration != 0) then {
		[
			{
				params ["_unit", "_target", "_camera", "_disableUserInput", "_disableDamage", "_setCaptive", "_interruptType", "_event", "_resetPosition", "_resetRotation", "_attachObject", "_cameraObject", "_cameraTarget", "_targetHitHandler", "_unitHitHandler", "_targetGroupHitHandler", "_unitGroupHitHandler", "_interruptCheck", "_animationId"];
				
				if !(missionNamespace getVariable [_animationId, false]) then {
					if (_interruptCheck isNotEqualTo []) then {
						[_interruptCheck] call KH_fnc_removeEventHandler;
					};

					[_event, [_unit, _target, true]] call CBA_fnc_globalEvent;

					if _disableDamage then {
						[
							[_unit],
							{
								params ["_unit"];
								_unit allowDamage true;
							},
							_unit,
							true
						] call KH_fnc_execute;
					};
					
					if ((alive _unit) && _setCaptive) then {
						[
							[_unit],
							{
								params ["_unit"];
								_unit setCaptive false;
							},
							_unit,
							true
						] call KH_fnc_execute;
					};
					
					if ((isPlayer _unit) && _disableUserInput) then {
						[[true, false], "KH_fnc_toggleUserInput", _unit, true] call KH_fnc_execute;
					};
					
					detach _unit;

					[
						[_unit],
						{
							params ["_unit"];
							_unit switchMove [""];
						},
						"GLOBAL",
						true
					] call KH_fnc_execute;

					[_unit, [_resetPosition, "ATL", false], [_resetRotation, false]] call KH_fnc_setTransforms;
					deleteVehicle _attachObject;
					
					if _camera then {
						deleteVehicle _cameraObject;
						deleteVehicle _cameraTarget;
					};
					
					if (!(isPlayer _unit) && (alive _unit) && (_interruptType == "NONE")) then {
						[
							[_unit],
							{
								params ["_unit"];
								_unit enableAI "ALL";
							},
							_unit,
							true
						] call KH_fnc_execute;
					};
					
					if (_targetHitHandler isNotEqualTo []) then {
						[_targetHitHandler] call KH_fnc_removeEventHandler;
					};
				
					if (_unitHitHandler isNotEqualTo []) then {
						[_unitHitHandler] call KH_fnc_removeEventHandler;
					};

					if (_targetGroupHitHandler isNotEqualTo []) then {
						[_targetGroupHitHandler] call KH_fnc_removeEventHandler;
					};
				
					if (_unitGroupHitHandler isNotEqualTo []) then {
						[_unitGroupHitHandler] call KH_fnc_removeEventHandler;
					};
				};
			},
			[_unit, _target, _camera, _disableUserInput, _disableDamage, _setCaptive, _interruptType, _event, _resetPosition, _resetRotation, _attachObject, _cameraObject, _cameraTarget, _targetHitHandler, _unitHitHandler, _targetGroupHitHandler, _unitGroupHitHandler, _interruptCheck, _animationId],
			_duration
		] call CBA_fnc_waitAndExecute;
	};
}
else {
	[
		[_unit, _animation],
		{
			params ["_unit", "_animation"];
			_unit switchMove _animation;
		},
		["JIP", "GLOBAL", _unit, false, false, ""],
		true
	] call KH_fnc_execute;

	_unit enableSimulationGlobal false;
	_unit setDamage 1;
};

true;