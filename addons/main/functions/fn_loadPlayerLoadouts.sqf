params ["_loadouts", ["_respawnType", "INITIAL"]];
KH_var_respawnLoadoutType = _respawnType;

if (_loadouts isEqualType []) then {
	KH_var_savedLoadoutsMap = createHashMapFromArray _loadouts;
}
else {
	KH_var_savedLoadoutsMap = createHashMapFromArray (missionProfileNamespace getVariable [format ["KH_var_savedPlayerLoadouts_%1", _loadouts], []]);
};

if (isNil "KH_var_initialPlayerLoadouts") then {
	KH_var_initialPlayerLoadouts = createHashMap;
};

if (isNil "KH_var_deathPlayerLoadouts") then {
	KH_var_deathPlayerLoadouts = createHashMap;
};

{
	private _uid = getPlayerUID _x;		
	private _loadout = KH_var_savedLoadoutsMap get _uid;
	private _initialLoadout = KH_var_initialPlayerLoadouts get _uid;
		
	if !(isNil "_loadout") then {
		_x setUnitLoadout _loadout;
	};

	if (isNil "_initialLoadout") then {
		private _currentLoadout = getUnitLoadout _x;
		KH_var_initialPlayerLoadouts insert [[_uid, _currentLoadout]];
	};
} forEach KH_var_allPlayerUnits;

if (isNil "KH_var_loadoutsSet") then {
	KH_var_loadoutsSet = true;

	[
		"KH_eve_playerRespawned", 
		{
			if (KH_var_respawnLoadoutType != "NONE") then {
				params ["_player"];
				private _uid = getPlayerUID _player;	
				private _loadout = KH_var_savedLoadoutsMap get _uid;
				private _initialLoadout = KH_var_initialPlayerLoadouts get _uid;
				private _deathLoadout = KH_var_deathPlayerLoadouts get _uid;

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
			params ["_player"];
			private _uid = getPlayerUID _player;		
			private _loadout = KH_var_savedLoadoutsMap get _uid;
			private _initialLoadout = KH_var_initialPlayerLoadouts get _uid;
			
			if !(_uid in KH_var_disconnectedPlayers) then {	
				if !(isNil "_loadout") then {
					_player setUnitLoadout _loadout;
				};

				if !(isNil "_initialLoadout") then {
					if (isNil "_loadout") then {
						_player setUnitLoadout _initialLoadout;
					};
				}
				else {
					private _currentLoadout = getUnitLoadout _player;
					KH_var_initialPlayerLoadouts insert [[_uid, _currentLoadout]];
				};
			};
		}
	] call CBA_fnc_addEventHandler;

	[
		"KH_eve_playerKilled", 
		{
			params ["_player"];
			private _uid = getPlayerUID _player;
			private _loadout = getUnitLoadout _player;	
			private _deathLoadout = KH_var_deathPlayerLoadouts get _uid;

			if !(isNil "_deathLoadout") then {
				KH_var_deathPlayerLoadouts set [_uid, _loadout];
			}
			else {
				KH_var_deathPlayerLoadouts insert [[_uid, _loadout]];
			};
		}
	] call CBA_fnc_addEventHandler;
};

true;