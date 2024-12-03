params ["_state", "_group", "_loadout", "_transforms", "_vehicle"];

KH_var_recoverDisconnectedPlayersGroup = _group;
KH_var_recoverDisconnectedPlayersLoadout = _loadout;
KH_var_recoverDisconnectedPlayersTransforms = _transforms;
KH_var_recoverDisconnectedPlayersVehicle = _vehicle;

if _state then {
	KH_var_recoverDisconnectedPlayers = true;
	
	if (isNil "KH_var_recoverDisconnectedPlayersSet") then {
		KH_var_recoverDisconnectedPlayersSet = true;

		[
			"KH_eve_playerDisconnected", 
			{
				if KH_var_recoverDisconnectedPlayers then {	
					private _attributes = _this select 1;
					private _uid = _this select 3;
					
					[
						"KH_eve_playerLoaded", 
						{
							params ["_unit"];
							_thisArgs params ["_attributes", "_oldUid"];
							private _newUid = getPlayerUID _unit;

							if (_oldUid == _newUid) then {
								[_unit, _attributes, KH_var_recoverDisconnectedPlayersGroup, false, true, KH_var_recoverDisconnectedPlayersLoadout, KH_var_recoverDisconnectedPlayersTransforms, KH_var_recoverDisconnectedPlayersTransforms, KH_var_recoverDisconnectedPlayersVehicle, true, true, false, true] call KH_fnc_setUnitAttributes;
								[_thisType, _thisId] call CBA_fnc_removeEventHandler;
							};
						},
						[_attributes, _uid]
					] call CBA_fnc_addEventHandlerArgs;
				};
			}
		] call CBA_fnc_addEventHandler;
	};
}
else {
	KH_var_recoverDisconnectedPlayers = false;
};

true;