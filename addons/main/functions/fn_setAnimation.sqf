params ["_unit", "_animation", "_position", "_rotation", "_target", "_positionReset", "_rotationReset", "_duration", ["_camera", false], ["_disableInput", true], ["_disableDamage", true], ["_setCaptive", false], ["_stopOnHit", false], ["_event", "KH_eve_scriptedAnimationFinished"]];

if (_duration != 0) then {
	private _targetHitHandler = -1;
	private _unitHitHandler = -1;
	private _resetPosition = getPosATL _positionReset;
	private _resetRotation = vectorDir _rotationReset;
	private _cameraObject = objNull;
	private _cameraTarget = objNull;
	private _attachObject = KH_var_logicGroup createUnit ["Logic", [0, 0, 0], [], 0, "CAN_COLLIDE"];
	[_attachObject, [_position, "ATL", false], [_rotation, false]] call KH_fnc_setTransforms;

	if (isPlayer _unit) then {
		[[[], true], "KH_fnc_closeInterface", _unit, "THIS_FRAME"] call KH_fnc_execute;
		
		if _disableInput then {
			[[false, false], "KH_fnc_toggleUserInput", _unit, "THIS_FRAME"] call KH_fnc_execute;
		};

		if _camera then {
			_cameraObject = createVehicle ["KH_HelperSquare", _x, [], 0, "CAN_COLLIDE"];
			_cameraObject attachTo [_unit, [-0.09, 0.01, 0.09], "Head", true];	
			_cameraTarget = createVehicle ["KH_HelperSquare", _x, [], 0, "CAN_COLLIDE"];
			_cameraTarget attachTo [_unit, [-3, 15, 0], "Head", true];
			[[[_unit], [_cameraObject], [_cameraTarget], [0.75], [0], [_duration], [-1], false, false], "KH_fnc_cameraSequence", _unit, "THIS_FRAME"] call KH_fnc_execute;

		};
	}
	else {
		[
			[_unit],
			{
				params ["_unit"];
				_unit disableAI "ALL";
			},
			_unit,
			"THIS_FRAME"
		] call KH_fnc_execute;
	};

	if _disableDamage then {
		[
			[_unit],
			{
				params ["_unit"];
				_unit allowDamage false;
			},
			_unit,
			"THIS_FRAME"
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
			"THIS_FRAME"
		] call KH_fnc_execute;
	};
	
	if _stopOnHit then
	{		
		_targetHitHandler = _target addEventHandler [
			"Dammaged", 
			{
				params ["_unit"];
				_unit setVariable ["KH_var_setAnimationUnitHit", true];
				_unit removeEventHandler [_thisEvent, _thisEventHandler];
			}
		];

		_unitHitHandler = _unit addEventHandler [
			"Dammaged", 
			{
				params ["_unit"];
				_unit setVariable ["KH_var_setAnimationUnitHit", true];
				_unit removeEventHandler [_thisEvent, _thisEventHandler];
			}
		];
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
		"THIS_FRAME"
	] call KH_fnc_execute;
		
	private _interruptCheck = [
		{
			_args params ["_unit", "_target", "_camera", "_disableInput", "_disableDamage", "_setCaptive", "_stopOnHit", "_event", "_resetPosition", "_resetRotation", "_attachObject", "_cameraObject", "_cameraTarget", "_targetHitHandler", "_unitHitHandler"];
			
			if (!(alive _unit) || !(alive _target) || (_unit getVariable ["KH_var_setAnimationUnitHit", false]) || (_target getVariable ["KH_var_setAnimationUnitHit", false])) then {	
				[_event, [_unit, _target, false]] call CBA_fnc_globalEvent;

				if _disableDamage then {
					[
						[_unit],
						{
							params ["_unit"];
							_unit allowDamage true;
						},
						_unit,
						"THIS_FRAME"
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
						"THIS_FRAME"
					] call KH_fnc_execute;
				};
				
				if ((isPlayer _unit) && _disableInput) then {
					[[true, false], "KH_fnc_toggleUserInput", _unit, "THIS_FRAME"] call KH_fnc_execute;
				};
				
				detach _unit;

				[
					[_unit],
					{
						params ["_unit"];
						_unit switchMove [""];
					},
					"GLOBAL",
					"THIS_FRAME"
				] call KH_fnc_execute;

				[_unit, [_resetPosition, "ATL", false], [_resetRotation, false]] call KH_fnc_setTransforms;
				deleteVehicle _attachObject;
				
				if _camera then {
					deleteVehicle _cameraObject;
					deleteVehicle _cameraTarget;
				};
				
				if (!(isPlayer _unit) && (alive _unit)) then {
					[
						[_unit],
						{
							params ["_unit"];
							_unit enableAI "ALL";
						},
						_unit,
						"THIS_FRAME"
					] call KH_fnc_execute;
				};
				
				if _stopOnHit then {
					if (_targetHitHandler != -1) then {
						_target removeEventHandler ["Dammaged", _targetHitHandler];
					};
				
					if (_unitHitHandler != -1) then {
						_unit removeEventHandler ["Dammaged", _unitHitHandler];
					};
				};
				
				_target setVariable ["KH_var_setAnimationUnitHit", true];
				_unit setVariable ["KH_var_setAnimationUnitHit", true];	
				[_handle] call CBA_fnc_removePerFrameHandler;
			};
		}, 
		0, 
		[_unit, _target, _camera, _disableInput, _disableDamage, _setCaptive, _stopOnHit, _event, _resetPosition, _resetRotation, _attachObject, _cameraObject, _cameraTarget, _targetHitHandler, _unitHitHandler]
	] call CBA_fnc_addPerFrameHandler;
	
	private _endFunction = {
		params ["_unit", "_target", "_camera", "_disableInput", "_disableDamage", "_setCaptive", "_stopOnHit", "_event", "_resetPosition", "_resetRotation", "_attachObject", "_cameraObject", "_cameraTarget", "_targetHitHandler", "_unitHitHandler", "_interruptCheck"];
		
		if (!(_unit getVariable ["KH_var_setAnimationUnitHit", false]) && !(_target getVariable ["KH_var_setAnimationUnitHit", false])) then {
			[_interruptCheck] call CBA_fnc_removePerFrameHandler;
			[_event, [_unit, _target, true]] call CBA_fnc_globalEvent;

			if _disableDamage then {
				[
					[_unit],
					{
						params ["_unit"];
						_unit allowDamage true;
					},
					_unit,
					"THIS_FRAME"
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
					"THIS_FRAME"
				] call KH_fnc_execute;
			};
			
			if ((isPlayer _unit) && _disableInput) then {
				[[true, false], "KH_fnc_toggleUserInput", _unit, "THIS_FRAME"] call KH_fnc_execute;
			};
			
			detach _unit;

			[
				[_unit],
				{
					params ["_unit"];
					_unit switchMove [""];
				},
				"GLOBAL",
				"THIS_FRAME"
			] call KH_fnc_execute;

			[_unit, [_resetPosition, "ATL", false], [_resetRotation, false]] call KH_fnc_setTransforms;
			deleteVehicle _attachObject;
			
			if _camera then {
				deleteVehicle _cameraObject;
				deleteVehicle _cameraTarget;
			};
			
			if (!(isPlayer _unit) && (alive _unit)) then {
				[
					[_unit],
					{
						params ["_unit"];
						_unit enableAI "ALL";
					},
					_unit,
					"THIS_FRAME"
				] call KH_fnc_execute;
			};
			
			if _stopOnHit then {
				if (_targetHitHandler != -1) then {
					_target removeEventHandler ["Dammaged", _targetHitHandler];
				};
				
				if (_unitHitHandler != -1) then {
					_unit removeEventHandler ["Dammaged", _unitHitHandler];
				};
			};
		};

		_target setVariable ["KH_var_setAnimationUnitHit", false];
		_unit setVariable ["KH_var_setAnimationUnitHit", false];
	};

	if (_duration != -1) then {
		[
			_endFunction,	
			[_unit, _target, _camera, _disableInput, _disableDamage, _setCaptive, _stopOnHit, _event, _resetPosition, _resetRotation, _attachObject, _cameraObject, _cameraTarget, _targetHitHandler, _unitHitHandler, _interruptCheck],
			_duration
		] call CBA_fnc_waitAndExecute;
	}
	else {
		[
			["STANDARD", _unit, false],
			"AnimDone",
			[_unit, _target, _camera, _disableInput, _disableDamage, _setCaptive, _stopOnHit, _event, _resetPosition, _resetRotation, _attachObject, _cameraObject, _cameraTarget, _targetHitHandler, _unitHitHandler, _interruptCheck, _endFunction, _animation],
			{
				params ["_unit", "_animation"];
				private _setAnimation = _args select 16;

				if (_setAnimation isEqualType []) then {
					_setAnimation = _setAnimation select 0;
				};

				if (_animation == _setAnimation) then {
					private _function = _args select 17;
					_args deleteAt [16, 17];
					_args call _function;
					_unit removeEventHandler [_thisEvent, _thisEventHandler];
				};
			}
		] call KH_fnc_addEventHandler;
	};
}
else {
	[
		[_unit, _animation],
		{
			params ["_unit", "_animation"];
			_unit switchMove _animation;
		},
		["JIP", "GLOBAL", _unit, false, false],
		"THIS_FRAME"
	] call KH_fnc_execute;

	_unit enableSimulationGlobal false;
	_unit setDamage 1;
};

true;