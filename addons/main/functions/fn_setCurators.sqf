params ["_curators", ["_modules", allCurators], ["_hide", true], ["_disableDamage", true]];
KH_var_curators = _curators;
KH_var_curatorModules = _modules;
KH_var_curatorsHidden = _hide;
KH_var_curatorDamageDisabled = _disableDamage;

private _curatorFunction = {
	params ["_player"];
	private _uid = getPlayerUID _player;

	if (_uid in KH_var_curators) then {
		private _module = KH_var_curatorModules select (KH_var_curators find _uid);

		if !((owner _player) in KH_var_allCuratorMachines) then {
			["KH_eve_curatorAssigned", [_player, owner _player]] call CBA_fnc_globalEvent;
		};

		KH_var_allCuratorMachines pushBackUnique (owner _player);
		publicVariable "KH_var_allCuratorMachines";
		
		if !(isNull (getAssignedCuratorUnit _module)) then {
			unassignCurator _module;
		};

		private _timeout = CBA_missionTime + 60;

		[
			{
				_args params ["_player", "_module", "_timeout"];
				private _finished = false;

				if (((getAssignedCuratorUnit _module) != _player) || ((getAssignedCuratorLogic _player) != _module)) then {
					if (!(isNull (getAssignedCuratorUnit _module)) && (isNull (getAssignedCuratorLogic _player))) then {
						unassignCurator _module;
					};
					
					_player assignCurator _module;
				}
				else {
					_finished = true;

					if KH_var_curatorsHidden then {
						_player hideObjectGlobal true;
					};

					if KH_var_curatorDamageDisabled then {
						[
							[],
							{
								player allowDamage false;
							},
							_player,
							true
						] call KH_fnc_execute;
					};

					[_handle] call CBA_fnc_removePerFrameHandler;
				};

				if ((CBA_missionTime > _timeout) && !_finished) then {
					[_handle] call CBA_fnc_removePerFrameHandler;
				};
			}, 
			1, 
			[_player, _module, _timeout]
		] call CBA_fnc_addPerFrameHandler;
	}
	else {
		{
			if ((getAssignedCuratorUnit _x) == _player) then {
				unassignCurator _x;
			};
		} forEach KH_var_curatorModules;
	};		
};

if (isNil "KH_var_curatorsSet") then {
	KH_var_curatorsSet = true;

	{
		[_x] call _curatorFunction;
	} forEach KH_var_allPlayerUnits;

	private _allEvents = ["KH_eve_playerRespawned", "KH_eve_playerLoaded", "KH_eve_playerSwitched"];

	{
		[_x, _curatorFunction] call CBA_fnc_addEventHandler;
	} forEach _allEvents;

	[
		"KH_eve_playerDisconnected", 
		{
			private _uid = _this select 3;
			private _uidIndex = KH_var_curators find _uid;

			if (_uidIndex != -1) then {
				unassignCurator (KH_var_curatorModules select _uidIndex);
			};
		}
	] call CBA_fnc_addEventHandler;
};

true;