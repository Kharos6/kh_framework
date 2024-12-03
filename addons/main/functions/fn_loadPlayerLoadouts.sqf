params ["_loadouts", ["_respawnType", "NONE"]];

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
				
				switch true do {
					case (KH_var_respawnLoadoutType == "INITIAL"): {
						if !(isNil "_initialLoadout") then {
							_player setUnitLoadout _initialLoadout;
						};
					};

					case (KH_var_respawnLoadoutType == "SAVED"): {
						if !(isNil "_loadout") then {
							_player setUnitLoadout _loadout;
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
					private _currentLoadout = getUnitLoadout _x;
					KH_var_initialPlayerLoadouts insert [[_uid, _currentLoadout]];
				};
			};
		}
	] call CBA_fnc_addEventHandler;
};

true;