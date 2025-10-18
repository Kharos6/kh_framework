params [["_audio", "", ["", []]], ["_condition", {}, [{}]], ["_interval", 0, [0]], ["_chance", 1, [0]]];

[
	[_audio, _condition, _chance],
	{
		params ["_audio", "_condition", "_chance"];

		if ((random 1) <= _chance) then {
			private _selectedAudio = selectRandom _audio;

			{
				if ([_x, _selectedAudio] call _condition) then {
					[
						[_selectedAudio],
						{
							params ["_selectedAudio"];
							playSound _selectedAudio;
						},
						_x,
						true,
						false
					] call KH_fnc_execute;
				};
			} forEach KH_var_allPlayerUnits;
		};
	},
	true,
	_interval,
	false
] call KH_fnc_execute;