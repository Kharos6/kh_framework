isNil {
	params [["_logic", objNull], ["_units", []], ["_activated", false]];

	if _activated then {
		private _parsedPositions = [];
		private _parsedTargets = [];

		{
			private _position = "";

			if ((_x select 0) isEqualType []) then {
				_position = _x select 0;
			}
			else {
				_position = missionNamespace getVariable [_x select 0, objNull];
			};

			private _attach = _x select 1;
			_parsedPositions pushBack [_position, _attach];
		} forEach (parseSimpleArray (_logic getVariable ["KH_ModuleCameraSequencePositions", "[]"]));

		{
			private _target = "";

			if (_x isEqualType []) then {
				_target = _x;
			}
			else {
				_target = missionNamespace getVariable [_x, objNull];
			};

			_parsedTargets pushBack _target;
		} forEach (parseSimpleArray (_logic getVariable ["KH_ModuleCameraSequenceTargets", "[]"]));

		{
			if (isPlayer _x) then {
				[
					[
						_parsedPositions, 
						_parsedTargets, 
						parseSimpleArray (_logic getVariable ["KH_ModuleCameraSequenceFOVs", "[]"]),
						parseSimpleArray (_logic getVariable ["KH_ModuleCameraSequenceCommitTimes", "[]"]),
						parseSimpleArray (_logic getVariable ["KH_ModuleCameraSequenceDurations", "[]"]),
						parseSimpleArray (_logic getVariable ["KH_ModuleCameraSequenceVisionTypes", "[]"]), 
						_logic getVariable ["KH_ModuleCameraSequenceCinematicBorders", false],
						_logic getVariable ["KH_ModuleCameraSequenceDisableUserInput", false]
					], 
					'KH_fnc_cameraSequence', 
					_x, 
					'THIS_FRAME'
				] call KH_fnc_execute;
			};
		} forEach _units;
	};
};

true;