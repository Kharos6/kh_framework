params [["_curators", [], [[]]], ["_modules", allCurators, [[]]], ["_hide", true, [true]], ["_disableDamage", true, [true]]];
KH_var_curators = createHashMap;
KH_var_curatorModules = [];

{
	private _module = _modules param [_forEachIndex];

	if (isNil "_module") then {
		_module = (createGroup [sideLogic, true]) createUnit ["ModuleCurator_F", [0, 0, 0], [], 0, "CAN_COLLIDE"];
		_module setVariable ["Addons", 3, true];
		_module setVariable ["BIS_fnc_initModules_disableAutoActivation", false];
		_module setCuratorCoef ["Place", 0];
		_module setCuratorCoef ["Edit", 0];
		_module setCuratorCoef ["Delete", 0];
		_module setCuratorCoef ["Destroy", 0];
		_module setCuratorCoef ["Group", 0];
		_module setCuratorCoef ["Synchronize", 0];
	};

	if (isNull _module) then {
		_module = (createGroup [sideLogic, true]) createUnit ["ModuleCurator_F", [0, 0, 0], [], 0, "CAN_COLLIDE"];
		_module setVariable ["Addons", 3, true];
		_module setVariable ["BIS_fnc_initModules_disableAutoActivation", false];
		_module setCuratorCoef ["Place", 0];
		_module setCuratorCoef ["Edit", 0];
		_module setCuratorCoef ["Delete", 0];
		_module setCuratorCoef ["Destroy", 0];
		_module setCuratorCoef ["Group", 0];
		_module setCuratorCoef ["Synchronize", 0];
	};

	KH_var_curators set [_x, _module];
	KH_var_curatorModules pushBack _module;
} forEach _curators;

KH_var_curatorsHidden = _hide;
KH_var_curatorsDamageDisabled = _disableDamage;

if (isNil "KH_var_curatorsSet") then {
	KH_var_curatorsSet = true;

	{
		private _player = _x;
		private _uid = getPlayerUID _player;
		private _module = KH_var_curators get _uid;

		if (isNil "_module") then {
			_module = KH_var_curators get (vehicleVarName _player);
		};

		if !(isNil "_module") then {
			if (isNull _module) then {
				_module = (createGroup [sideLogic, true]) createUnit ["ModuleCurator_F", [0, 0, 0], [], 0, "CAN_COLLIDE"];
				_module setVariable ["Addons", 3, true];
				_module setVariable ["BIS_fnc_initModules_disableAutoActivation", false];
				_module setCuratorCoef ["Place", 0];
				_module setCuratorCoef ["Edit", 0];
				_module setCuratorCoef ["Delete", 0];
				_module setCuratorCoef ["Destroy", 0];
				_module setCuratorCoef ["Group", 0];
				_module setCuratorCoef ["Synchronize", 0];
				KH_var_curators set [_uid, _module];
			};

			if !(isNull (getAssignedCuratorUnit _module)) then {
				unassignCurator _module;
			};

			[
				[_player, _module],
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
								true,
								false
							] call KH_fnc_execute;
						};

						if !((owner _player) in KH_var_allCuratorMachines) then {
							["KH_eve_curatorAssigned", [owner _player, getPlayerUID _player, _player], "GLOBAL", false] call KH_fnc_triggerCbaEvent;
						};

						KH_var_allCuratorMachines pushBackUnique (owner _player);
						publicVariable "KH_var_allCuratorMachines";
						[_handlerId] call KH_fnc_removeHandler;
					};
				},
				true,
				[1, true, 60, {}, false, true],
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
	} forEach KH_var_allPlayerUnits;

	{
		[
			"CBA",
			_x,
			[],
			{
				private _uid = param [1];
				private _player = param [3];
				private _module = KH_var_curators get _uid;

				if (isNil "_module") then {
					_module = KH_var_curators get (vehicleVarName _player);
				};

				if !(isNil "_module") then {
					if (isNull _module) then {
						_module = (createGroup [sideLogic, true]) createUnit ["ModuleCurator_F", [0, 0, 0], [], 0, "CAN_COLLIDE"];
						_module setVariable ["Addons", 3, true];
						_module setVariable ["BIS_fnc_initModules_disableAutoActivation", false];
						_module setCuratorCoef ["Place", 0];
						_module setCuratorCoef ["Edit", 0];
						_module setCuratorCoef ["Delete", 0];
						_module setCuratorCoef ["Destroy", 0];
						_module setCuratorCoef ["Group", 0];
						_module setCuratorCoef ["Synchronize", 0];
						KH_var_curators set [_uid, _module];
					};

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
									["KH_eve_curatorAssigned", [owner _player, getPlayerUID _player, _player], "GLOBAL", false] call KH_fnc_triggerCbaEvent;
								};

								KH_var_allCuratorMachines pushBackUnique (owner _player);
								publicVariable "KH_var_allCuratorMachines";
								[_handlerId] call KH_fnc_removeHandler;
							};
						},
						true,
						[1, true, 60, {}, false, true],
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
		] call KH_fnc_addEventHandler;
	} forEach ["KH_eve_playerRespawned", "KH_eve_playerLoaded", "KH_eve_playerSwitched"];

	[
		"CBA",
		"KH_eve_playerDisconnected",
		[], 
		{
			private _uid = param [1];
			private _player = param [3];
			private _module = KH_var_curators get _uid;

			if (isNil "_module") then {
				_module = KH_var_curators get (vehicleVarName _player);
			};

			if !(isNil "_module") then {
				unassignCurator _module;
			};
		}
	] call KH_fnc_addEventHandler;
};

nil;