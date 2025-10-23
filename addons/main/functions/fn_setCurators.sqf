params [["_curators", [], [[]]], ["_modules", allCurators, [[]]], ["_hide", true, [true]], ["_disableDamage", true, [true]]];
KH_var_curators = _curators;
KH_var_curatorModules = _modules;
KH_var_curatorsHidden = _hide;
KH_var_curatorsDamageDisabled = _disableDamage;

if (isNil "KH_var_curatorsSet") then {
	KH_var_curatorsSet = true;

	{
		private _uid = getPlayerUID _x;

		if (_uid in KH_var_curators) then {
			private _module = KH_var_curatorModules select (KH_var_curators find _uid);
			
			if !(isNull (getAssignedCuratorUnit _module)) then {
				unassignCurator _module;
			};

			[
				[_x, _module],
				{
					params ["_player", "_module"];

					if (((getAssignedCuratorUnit _module) isNotEqualTo _player) || ((getAssignedCuratorLogic _player) isNotEqualTo _module)) then {
						if (!(isNull (getAssignedCuratorUnit _module)) && (isNull (getAssignedCuratorLogic _player))) then {
							unassignCurator _module;
						};
						
						_player assignCurator _module;
					}
					else {
						if KH_var_curatorsHidden then {
							_player hideObjectGlobal true;
						};

						if KH_var_curatorsDamageDisabled then {
							[
								[],
								{
									player allowDamage false;
								},
								_player,
								true
							] call KH_fnc_execute;
						};

						if !((owner _player) in KH_var_allCuratorMachines) then {
							["KH_eve_curatorAssigned", [owner _player, getPlayerUID _player, _player]] call CBA_fnc_globalEvent;
						};

						KH_var_allCuratorMachines pushBackUnique (owner _player);
						publicVariable "KH_var_allCuratorMachines";
						[_handlerId] call KH_fnc_removeHandler;
					};
				},
				true,
				[1, true, 60, true],
				false 
			] call KH_fnc_execute;
		}
		else {
			{
				if ((getAssignedCuratorUnit _x) isEqualTo _x) then {
					unassignCurator _x;
				};
			} forEach KH_var_curatorModules;
		};
	} forEach KH_var_allPlayerUnits;

	{
		[
			_x,
			{
				private _player = param [3];
				private _uid = getPlayerUID _player;

				if (_uid in KH_var_curators) then {
					private _module = KH_var_curatorModules select (KH_var_curators find _uid);
					
					if !(isNull (getAssignedCuratorUnit _module)) then {
						unassignCurator _module;
					};

					[
						[_player, _module, _timeout],
						{
							params ["_player", "_module", "_timeout"];

							if (((getAssignedCuratorUnit _module) isNotEqualTo _player) || ((getAssignedCuratorLogic _player) isNotEqualTo _module)) then {
								if (!(isNull (getAssignedCuratorUnit _module)) && (isNull (getAssignedCuratorLogic _player))) then {
									unassignCurator _module;
								};
								
								_player assignCurator _module;
							}
							else {
								if KH_var_curatorsHidden then {
									_player hideObjectGlobal true;
								};

								if KH_var_curatorsDamageDisabled then {
									[
										[],
										{
											player allowDamage false;
										},
										_player,
										true
									] call KH_fnc_execute;
								};

								if !((owner _player) in KH_var_allCuratorMachines) then {
									["KH_eve_curatorAssigned", [owner _player, getPlayerUID _player, _player]] call CBA_fnc_globalEvent;
								};

								KH_var_allCuratorMachines pushBackUnique (owner _player);
								publicVariable "KH_var_allCuratorMachines";
								[_handlerId] call KH_fnc_removeHandler;
							};
						},
						true,
						[1, true, 60, true],
						false 
					] call KH_fnc_execute;
				}
				else {
					{
						if ((getAssignedCuratorUnit _x) isEqualTo _player) then {
							unassignCurator _x;
						};
					} forEach KH_var_curatorModules;
				};
			}
		] call CBA_fnc_addEventHandler;
	} forEach ["KH_eve_playerRespawned", "KH_eve_playerLoaded", "KH_eve_playerSwitched"];

	[
		"KH_eve_playerDisconnected", 
		{
			private _uid = param [1];
			private _uidIndex = KH_var_curators find _uid;

			if (_uidIndex isNotEqualTo -1) then {
				unassignCurator (KH_var_curatorModules select _uidIndex);
			};
		}
	] call CBA_fnc_addEventHandler;
};

nil;