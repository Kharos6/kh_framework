params ["_positionEntities", "_targetEntities", "_fovs", "_commitTimes", "_durations", "_visionTypes", ["_cinematicBorders", false], ["_disableUserInput", true]];
private _totalDuration = [_durations] call KH_fnc_arraySum;

if (alive player) then {
	[[], true] call KH_fnc_closeInterface;

	if _disableUserInput then {
		[false, true] call KH_fnc_toggleUserInput;
	};

	private _playerParent = objectParent player;

	if !(isNull _playerParent) then {
		if ((driver _playerParent) == player) then {
			[
				[_playerParent, _totalDuration],
				{
					params ["_vehicle", "_totalDuration"];
					_vehicle enableSimulationGlobal false;

					[
						{
							params ["_vehicle"];
							_vehicle enableSimulationGlobal true;
						}, 
						[_vehicle], 
						_totalDuration
					] call CBA_fnc_waitAndExecute;
				},
				"SERVER",
				"THIS_FRAME"
			] call KH_fnc_execute;
		};
	};

	private _camera = "camera" camCreate ([(_positionEntities select 0) select 0, "AGL", []] call KH_fnc_getPosition);
	_camera cameraEffect ["Internal", "BACK"];
	showCinemaBorder _cinematicBorders;
	private _cameraCount = (count _positionEntities) - 1;
	private _i = 0;
	
	for "_i" from 0 to _cameraCount do {
		private _duration = 0;

		if (_i != 0) then {
			_duration = [_durations select [0, _i]] call KH_fnc_arraySum;
		};	

		[
			{
				params ["_positionEntities", "_targetEntities", "_fovs", "_commitTimes", "_visionTypes", "_camera", "_i"];
				private _position = (_positionEntities select _i) select 0;
				private _attach = (_positionEntities select _i) select 1;
				private _visionType = _visionTypes select _i;

				if _attach then {
					_camera attachTo [_position, [0, 0, 0]];
				}
				else {
					_camera camSetPos ([_position, "AGL", []] call KH_fnc_getPosition);
				};

				_camera camSetTarget (_targetEntities select _i);
				_camera camSetFov (_fovs select _i);
				_camera camCommit (_commitTimes select _i);

				switch true do {
					case (_visionType == -2): {
						camUseNVG true;
					};

					case (_visionType == -1): {
						false setCamUseTI 0;
						camUseNVG false;
					};

					case (_visionType >= 0): {
						true setCamUseTI _visionType;
					};
				};
			}, 
			[_positionEntities, _targetEntities, _fovs, _commitTimes, _visionTypes, _camera, _i], 
			_duration
		] call CBA_fnc_waitAndExecute;
	};
	
	[
		{
			params ["_disableUserInput", "_camera"];
			_camera cameraEffect ["Terminate", "BACK"];
			camDestroy _camera;

			if _disableUserInput then {
				[true, true] call KH_fnc_toggleUserInput;
			};
		}, 
		[_disableUserInput, _camera], 
		_totalDuration
	] call CBA_fnc_waitAndExecute;
};

_totalDuration;