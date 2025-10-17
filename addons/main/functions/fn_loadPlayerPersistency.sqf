params [["_identifier", "", [""]], ["_loadoutRespawnType", "INITIAL", [""]], ["_useUid", true, [true]], ["_overrideAttributes", [], [[]]]];
if (_identifier isEqualTo "") exitWith {};
KH_var_playerPersistencyLoadoutRespawnType = _loadoutRespawnType;
KH_var_playerPersistencyUseUid = _useUid;
KH_var_playerPersistencyUid = "khNamespace" readKhData [["playerPersistencyUid_", _identifier] joinString "", createHashMap];
KH_var_playerPersistencyVariableName = "khNamespace" readKhData [["playerPersistencyVariableName_", _identifier] joinString "", createHashMap];

if (isNil "KH_var_initialPlayerLoadouts") then {
	KH_var_initialPlayerLoadouts = createHashMap;
};

if (isNil "KH_var_deathPlayerLoadouts") then {
	KH_var_deathPlayerLoadouts = createHashMap;
};

{
	private _uid = getPlayerUID _x;
	private _initialLoadout = KH_var_initialPlayerLoadouts get _uid;

	private _attributes = if KH_var_playerPersistencyUseUid then {
		if (isNil "_initialLoadout") then {
			KH_var_initialPlayerLoadouts set [_uid, getUnitLoadout _x];
		};

		KH_var_playerPersistencyUid get _uid;
	}
	else {
		private _variableName = vehicleVarName _x;

		if ((_variableName isNotEqualTo "") && !(_x getVariable ["KH_var_generatedVariableName", false])) then {
			if (isNil "_initialLoadout") then {
				KH_var_initialPlayerLoadouts set [_variableName, getUnitLoadout _x];
			};

			KH_var_playerPersistencyVariableName get _variableName;
		};
	};

	if !(isNil "_attributes") then {
		[_x, _attributes, _overrideAttributes, true] call KH_fnc_setUnitAttributes;
	};
} forEach KH_var_allPlayerUnits;

if (isNil "KH_var_loadoutsSet") then {
	KH_var_loadoutsSet = true;

	[
		"KH_eve_playerRespawned", 
		{
			if (KH_var_respawnLoadoutType isNotEqualTo "NONE") then {
				private _uid = param [1];
				private _player = param [3];
				private _initialLoadout = KH_var_initialPlayerLoadouts get _uid;
				private _deathLoadout = KH_var_deathPlayerLoadouts get _uid;

				private _loadout = if KH_var_playerPersistencyUseUid then {
					private _attributes = KH_var_playerPersistencyUid get _uid;

					if !(isNil "_attributes") then {
						(_attributes select 5) select 21;
					};
				}
				else {
					private _variableName = vehicleVarName _x;

					if ((_variableName isNotEqualTo "") && !(_x getVariable ["KH_var_generatedVariableName", false])) then {
						private _attributes = KH_var_playerPersistencyVariableName get _variableName;

						if !(isNil "_attributes") then {
							(_attributes select 5) select 21;
						};
					};
				};

				switch KH_var_respawnLoadoutType do {
					case "INITIAL": {
						if !(isNil "_initialLoadout") then {
							_player setUnitLoadout _initialLoadout;
						}
						else {
							if !(isNil "_deathLoadout") then {
								_player setUnitLoadout _deathLoadout;
							};
						};
					};

					case "SAVED": {
						if !(isNil "_loadout") then {
							_player setUnitLoadout _loadout;
						}
						else {
							if !(isNil "_initialLoadout") then {
								_player setUnitLoadout _initialLoadout;
							}
							else {
								if !(isNil "_deathLoadout") then {
									_player setUnitLoadout _deathLoadout;
								};
							};
						};
					};

					case "DEATH": {
						if !(isNil "_deathLoadout") then {
							_player setUnitLoadout _deathLoadout;
						}
						else {
							if !(isNil "_initialLoadout") then {
								_player setUnitLoadout _initialLoadout;
							};
						};
					};
				};
			};
		}
	] call CBA_fnc_addEventHandler;
	
	[
		"KH_eve_playerLoaded", 
		{
			private _uid = param [1];
			private _player = param [3];
			private _initialLoadout = KH_var_initialPlayerLoadouts get _uid;
			
			if !(_uid in KH_var_disconnectedPlayerUids) then {
				if KH_var_playerPersistencyUseUid then {
					private _attributes = KH_var_playerPersistencyUid get _uid;

					private _loadout = if !(isNil "_attributes") then {
						(_attributes select 5) select 21;
					};

					if !(isNil "_loadout") then {
						_player setUnitLoadout _loadout;
					};

					if !(isNil "_initialLoadout") then {
						if (isNil "_loadout") then {
							_player setUnitLoadout _initialLoadout;
						};
					}
					else {
						KH_var_initialPlayerLoadouts set [_uid, getUnitLoadout _player];
					};
				}
				else {
					private _variableName = vehicleVarName _player;

					if ((_variableName isNotEqualTo "") && !(_player getVariable ["KH_var_generatedVariableName", false])) then {
						private _attributes = KH_var_playerPersistencyVariableName get _variableName;

						private _loadout = if !(isNil "_attributes") then {
							(_attributes select 5) select 21;
						};

						if !(isNil "_loadout") then {
							_player setUnitLoadout _loadout;
						};

						if !(isNil "_initialLoadout") then {
							if (isNil "_loadout") then {
								_player setUnitLoadout _initialLoadout;
							};
						}
						else {
							KH_var_initialPlayerLoadouts set [_uid, getUnitLoadout _player];
						};
					};
				};
			};
		}
	] call CBA_fnc_addEventHandler;

	[
		"KH_eve_playerKilled", 
		{
			private _uid = param [1];
			private _player = param [3];	
			KH_var_deathPlayerLoadouts set [_uid, getUnitLoadout _player];
		}
	] call CBA_fnc_addEventHandler;
};

nil;