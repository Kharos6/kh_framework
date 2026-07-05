params [["_audio", [], [[]]], ["_condition", {true;}, [{}]], ["_interval", 0, [0]], ["_chance", 1, [0]]];

execute [
	[_audio, _condition, _chance],
	{
		params ["_audio", "_condition", "_chance"];

		if ((random 1) <= _chance) then {
			private _selectedAudio = selectRandom _audio;

			{
				if ([_x, _selectedAudio] call _condition) then {
					execute [
						[_selectedAudio],
						{
							params ["_selectedAudio"];
							playSound _selectedAudio;
						},
						_x,
						true,
						false
					];
				};
			} forEach KH_var_allPlayerUnits;
		};
	},
	true,
	_interval,
	false
];