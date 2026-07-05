params [["_emitters", [], [[]]], ["_audio", [], [[]]], ["_condition", {true;}, [{}]], ["_interval", 0, [0]], ["_chance", 1, [0]]];

execute [
	[_emitters, _audio, _condition, _chance],
	{
		params ["_emitters", "_audio", "_condition", "_chance"];

		{
			if ((random 1) <= _chance) then {
				private _emitter = _x;
				private _selectedAudio = selectRandom _audio;

				{
					if ([_x, _emitter, _selectedAudio] call _condition) then {
						execute [
							[_emitter, _selectedAudio],
							{
								params ["_emitter", "_selectedAudio"];
								_emitter say3D _selectedAudio;
							},
							_x,
							true,
							false
						];
					};
				} forEach KH_var_allPlayerUnits;
			};
		} forEach _emitters;
	},
	true,
	_interval,
	false
];