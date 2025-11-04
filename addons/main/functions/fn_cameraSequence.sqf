params [
	["_positions", [], [[]]], 
	["_targets", [], [[]]], 
	["_fovs", [], [[]]], 
	["_commitTimes", [], [[]]], 
	["_durations", [], [[]]], 
	["_visionTypes", [], [[]]], 
	["_cinematicBorders", false, [true]], 
	["_disableUserInput", true, [true]]
];

private _totalDuration = [_durations] call KH_fnc_arraySum;

if ((_durations select 0) isEqualTo 0) then {
	_durations set [0, 0.01];
};

if !(alive player) exitWith {
	0;
};

[displayNull, true] call KH_fnc_closeInterface;

if _disableUserInput then {
	[false, true, true] call KH_fnc_toggleUserInput;
};

private _playerParent = objectParent player;

if !(isNull _playerParent) then {
	if ((driver _playerParent) isEqualTo player) then {
		[
			[_playerParent, _totalDuration],
			{
				params ["_vehicle", "_totalDuration"];
				_vehicle enableSimulationGlobal false;

				[
					[_vehicle],
					{
						params ["_vehicle"];
						_vehicle enableSimulationGlobal true;
					}, 
					true, 
					str _totalDuration,
					false
				] call KH_fnc_execute;
			},
			"SERVER",
			true,
			false
		] call KH_fnc_execute;
	};
};

private _firstPosition = if ((_positions select 0) isEqualType []) then {
	if (((_positions select 0) select 0) isEqualType objNull) then {
		((_positions select 0) select 0) modelToWorldVisual [0, 0, 0];
	}
	else {
		_positions select 0;
	};
}
else {
	(_positions select 0) modelToWorldVisual [0, 0, 0];
};

private _camera = "camera" camCreate _firstPosition;
_camera cameraEffect ["Internal", "BACK"];
showCinemaBorder _cinematicBorders;
private _cameraCount = (count _positions) - 1;

for "_i" from 0 to _cameraCount do {
	private _duration = 0;

	if (_i isNotEqualTo 0) then {
		_duration = [_durations select [0, _i]] call KH_fnc_arraySum;
	};

	[
		[_positions, _targets, _fovs, _commitTimes, _visionTypes, _camera, _i],
		{
			params ["_positions", "_targets", "_fovs", "_commitTimes", "_visionTypes", "_camera", "_i"];
			private _position = _positions select _i;
			private _visionType = _visionTypes select _i;
			
			if (_position isEqualType []) then { 
				if ((_position select 0) isEqualType objNull) then {
					_position params [["_entity", objNull, [objNull]], ["_attach", false, [true]]];
					private _attach = (_positions select _i) select 1;
					
					if _attach then {
						_camera attachTo [_entity, [0, 0, 0]];
					}
					else {
						_camera camSetPos (_entity modelToWorldVisual [0, 0, 0]);
					};
				}
				else {
					_camera camSetPos _position;
				};
			}
			else {
				_camera camSetPos (_position modelToWorldVisual [0, 0, 0]);
			};

			_camera camSetTarget (_targets select _i);
			_camera camSetFov (_fovs select _i);
			_camera camCommit (_commitTimes select _i);

			switch _visionType do {
				case -2: {
					camUseNVG true;
				};

				case -1: {
					false setCamUseTI 0;
					camUseNVG false;
				};

				default {
					true setCamUseTI _visionType;
				};
			};
		},
		true,
		str _duration,
		false
	] call KH_fnc_execute;
};

[
	[_disableUserInput, _camera],
	{
		params ["_disableUserInput", "_camera"];
		_camera cameraEffect ["Terminate", "BACK"];
		camDestroy _camera;

		if _disableUserInput then {
			[true, true, true] call KH_fnc_toggleUserInput;
		};
	}, 
	true, 
	str _totalDuration,
	false
] call KH_fnc_execute;

_camera;