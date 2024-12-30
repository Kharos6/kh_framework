isNil {
	params [["_logic", objNull], ["_units", []], ["_activated", false]];

	if (_activated && hasInterface) then {
		[
			[_logic, _units],
			{
				params ["_logic", "_units"];
				private _remote = _logic getVariable ["KH_ModuleAddInteractionEntryRemote", false];
				private _name = _logic getVariable ["KH_ModuleAddInteractionEntryName", ""];
				private _tooltip = _logic getVariable ["KH_ModuleAddInteractionEntryTooltip", ""];
				private _condition = compile (_logic getVariable ["KH_ModuleAddInteractionEntryCondition", ""]);
				private _options = _logic getVariable ["KH_ModuleAddInteractionEntryOptions", "[]"];

				if (("[" in _options) || ("]" in _options)) then {
					_options = parseSimpleArray _options;
				}
				else {
					_options = missionNamespace getVariable [_options, []];
				};

				private _parsedOptions = [];
				
				{
					_parsedOptions pushBack [_x select 0, _x select 1, compile (_x select 2), [], compile (_x select 3)];
				} forEach _options;

				[
					"CBA",
					"KH_eve_playerLoaded",
					[_units, _remote, _name, _tooltip, _condition, _parsedOptions],
					{
						private _joiningMachineId = _this select 1;
						_args params ["_units", "_remote", "_name", "_tooltip", "_condition", "_options"];


						if (clientOwner == _joiningMachineId) then {
							if _remote then {
								{
									if (player == _x) then {		
										[
											[_x, _name, _tooltip, _condition, _options],
											"KH_fnc_addRemoteInteractionEntry",
											["JIP", "PLAYERS", _x, false, true],
											"THIS_FRAME"
										] call KH_fnc_execute;
									}
									else {
										if !(isPlayer _x) then {
											[_x, _name, _tooltip, _condition, _options] call KH_fnc_addRemoteInteractionEntry;
										};
									};
								} forEach _units;
							}
							else {
								[_name, _tooltip, _condition, _options] call KH_fnc_addSelfInteractionEntry;
							};

							[_eventName, _localId] call CBA_fnc_removeEventHandler;
						};
					}
				] call KH_fnc_addEventHandler;
			},
			"LOCAL",
			"THIS_FRAME"
		] call KH_fnc_execute;
	};
};

true;