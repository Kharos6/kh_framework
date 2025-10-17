params [["_state", true, [true]]];
KH_var_recoverDisconnectedPlayers = _state;

if _state then {	
	if (isNil "KH_var_recoverDisconnectedPlayersSet") then {
		KH_var_recoverDisconnectedPlayersSet = true;

		[
			"KH_eve_playerDisconnected", 
			{
				if KH_var_recoverDisconnectedPlayers then {	
					private _uid = param [1];
					private _attributes = param [4];
					
					if (_attributes isNotEqualTo []) then {
						[
							"CBA",
							"KH_eve_playerLoaded",
							[_attributes, _uid],
							{
								private _newUid = param [1];
								private _unit = param [3];
								_args params ["_attributes", "_oldUid"];

								if (_oldUid isEqualTo _newUid) then {
									[_unit, _attributes, [], true] call KH_fnc_setUnitAttributes;
									[_handlerId] call KH_fnc_removeHandler;
								};
							}
						] call KH_fnc_addEventHandler;
					};
				};
			}
		] call CBA_fnc_addEventHandler;
	};
};

nil;