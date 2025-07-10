params ["_emitters", "_audio", "_condition", "_interval", "_randomDelay", "_chance"];
private _id = [missionNamespace, "KH_var_3dAudio", "ACTIVE", false] call KH_fnc_atomicVariable;

[
	{
		private _id = _args select 5;
		private _idState = missionNamespace getVariable [_id, "ACTIVE"];

		if (_idState isNotEqualTo "INACTIVE") then {
			switch _idState do {
				case "ACTIVE": {
					_args params ["_emitters", "_audio", "_condition", "_randomDelay", "_chance"];

					{
						if ((random 1) <= _chance) then {
							private _emitter = _x;
							private _selectedAudio = selectRandom _audio;

							{
								if (_randomDelay > 0) then {
									[
										{
											params ["_player", "_emitter", "_condition", "_id", "_selectedAudio"];

											if ([_player, _emitter, _selectedAudio, ["PRIVATE_HANDLER", _id, clientOwner]] call _condition) then {
												[
													[_emitter, _selectedAudio],
													{
														params ["_emitter", "_selectedAudio"];
														_emitter say3D _selectedAudio;
													},
													_player,
													"THIS_FRAME"
												] call KH_fnc_execute;
											};
										}, 
										[_x, _emitter, _condition, _id, _selectedAudio],
										random _randomDelay
									] call CBA_fnc_waitAndExecute;
								}
								else {
									if ([_x, _emitter, _selectedAudio, ["PRIVATE_HANDLER", _id, clientOwner]] call _condition) then {
										[
											[_emitter, _selectedAudio],
											{
												params ["_emitter", "_selectedAudio"];
												_emitter say3D _selectedAudio;
											},
											_x,
											"THIS_FRAME"
										] call KH_fnc_execute;
									};
								};
							} forEach KH_var_allPlayerUnits;
						};
					} forEach _emitters;
				};

				case "TERMINATE": {
					[_handle] call CBA_fnc_removePerFrameHandler;
				};		
			};
		};
	},
	_interval, 
	[_emitters, _audio, _condition, _randomDelay, _chance, _id]
] call CBA_fnc_addPerFrameHandler;

["PRIVATE_HANDLER", _id, clientOwner];