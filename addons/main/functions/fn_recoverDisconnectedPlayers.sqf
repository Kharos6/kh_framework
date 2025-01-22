params [["_state", true], ["_group", true], ["_loadout", true], ["_transforms", true], ["_vehicle", true]];
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
					private _attributes = _this select 2;
					private _uid = _this select 3;
					
					if (_attributes isNotEqualTo []) then {
						[
							"CBA",
							"KH_eve_playerLoaded",
							[_attributes, _uid],
							{
								params ["_unit"];
								_args params ["_attributes", "_oldUid"];
								private _newUid = getPlayerUID _unit;

								if (_oldUid == _newUid) then {
									[_unit, _attributes, KH_var_recoverDisconnectedPlayersGroup, false, true, KH_var_recoverDisconnectedPlayersLoadout, KH_var_recoverDisconnectedPlayersTransforms, KH_var_recoverDisconnectedPlayersTransforms, KH_var_recoverDisconnectedPlayersVehicle, true, true, false, true] call KH_fnc_setUnitAttributes;
									[_localId] call KH_fnc_removeEventHandler;
								};
							}
						] call KH_fnc_addEventHandler;
					};
				};
			}
		] call CBA_fnc_addEventHandler;
	};
}
else {
	KH_var_recoverDisconnectedPlayers = false;
};

true;