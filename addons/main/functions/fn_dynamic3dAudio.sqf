params ["_emitters", "_audio", "_condition", "_interval", "_chance"];
private _id = [missionNamespace, "KH_var_3dAudio", "ACTIVE", false] call KH_fnc_atomicVariable;

[
	{
		private _id = _args select 4;
		private _idState = missionNamespace getVariable [_id, "ACTIVE"];

		if (_idState != "INACTIVE") then {
			switch true do {
				case (_idState == "ACTIVE"): {
					_args params ["_emitters", "_audio", "_condition", "_chance"];

					{
						if ((random 1) <= _chance) then {
							private _emitter = _x;
							private _selectedAudio = selectRandom _audio;

							{
								if ([_x, _emitter, ["PRIVATE_HANDLER", _id, clientOwner], _selectedAudio] call _condition) then {
									[
										[_emitter, _selectedAudio],
										{
											params ["_emitter", "_selectedAudio"];
											_emitter say3D _audio;
										},
										_x,
										"THIS_FRAME"
									] call KH_fnc_execute;
								};
							} forEach KH_var_allPlayerUnits;
						};
					} forEach _emitters;
				};

				case (_idState == "TERMINATE"): {
					[_handle] call CBA_fnc_removePerFrameHandler;
				};		
			};
		};
	},
	_interval, 
	[_emitters, _audio, _condition, _chance, _id]
] call CBA_fnc_addPerFrameHandler;

["PRIVATE_HANDLER", _id, clientOwner];