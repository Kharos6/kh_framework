params ["_curators", ["_modules", allCurators], ["_hide", true], ["_disableDamage", true]];
KH_var_curators = _curators;
KH_var_curatorModules = _modules;
KH_var_curatorsHidden = _hide;
KH_var_curatorDamageDisabled = _disableDamage;

if (isNil "KH_var_curatorsSet") then {
	KH_var_curatorsSet = true;

	{
		private _uid = getPlayerUID _x;

		if (_uid in KH_var_curators) then {
			private _module = _modules select (KH_var_curators find _uid);
			
			if !(isNull (getAssignedCuratorUnit _module)) then {
				unassignCurator _module;
			};

			[
				{
					private _module = _this select 1;
					(isNull (getAssignedCuratorUnit _module));
				}, 
				{
					params ["_player", "_module"];

					[
						{
							params ["_player", "_module"];
							_player assignCurator _module;

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
									"THIS_FRAME"
								] call KH_fnc_execute;
							};
						}, 
						[_player, _module]
					] call CBA_fnc_execNextFrame;
				}, 
				[_x, _module],
				30
			] call CBA_fnc_waitUntilAndExecute;
		};
	} forEach KH_var_allPlayerUnits;

	private _allEvents = ["KH_eve_playerRespawned", "KH_eve_playerLoaded", "KH_eve_playerSwitched"];

	{
		[
			_x, 
			{
				params ["_player"];
				private _uid = getPlayerUID _player;

				if (_uid in KH_var_curators) then {
					private _module = KH_var_curatorModules select (KH_var_curators find _uid);

					if !(isNull (getAssignedCuratorUnit _module)) then {
						unassignCurator _module;
					};

					[
						{
							private _module = _this select 1;
							(isNull (getAssignedCuratorUnit _module));
						}, 
						{
							params ["_player", "_module"];

							[
								{
									params ["_player", "_module"];
									_player assignCurator _module;

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
											"THIS_FRAME"
										] call KH_fnc_execute;
									};
								}, 
								[_player, _module]
							] call CBA_fnc_execNextFrame;
						}, 
						[_player, _module],
						30
					] call CBA_fnc_waitUntilAndExecute;
				}
				else {
					{
						if ((getAssignedCuratorUnit _x) == _player) then {
							unassignCurator _x;
						};
					} forEach KH_var_curatorModules;
				};
			}
		] call CBA_fnc_addEventHandler;
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