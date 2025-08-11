params ["_audio", "_condition", "_interval", "_chance"];
private _id = [missionNamespace, "KH_var_2dAudio", "ACTIVE", false] call KH_fnc_atomicVariable;

[
	{
		private _id = _args select 3;
		private _idState = missionNamespace getVariable [_id, "ACTIVE"];

		if (_idState isNotEqualTo "INACTIVE") then {
			switch _idState do {
				case "ACTIVE": {
					_args params ["_audio", "_condition", "_chance"];
					
					if ((random 1) <= _chance) then {
						private _selectedAudio = selectRandom _audio;

						{
							if ([_x, _selectedAudio, ["PRIVATE_HANDLER", _id, clientOwner]] call _condition) then {
								[
									[_selectedAudio],
									{
										params ["_selectedAudio"];
										playSound _selectedAudio;
									},
									_x,
									true
								] call KH_fnc_execute;
							};
						} forEach KH_var_allPlayerUnits;
					};
				};

				case "TERMINATE": {
					[_handle] call CBA_fnc_removePerFrameHandler;
				};		
			};
		};
	},
	_interval, 
	[_audio, _condition, _chance, _id]
] call CBA_fnc_addPerFrameHandler;

["PRIVATE_HANDLER", _id, clientOwner];