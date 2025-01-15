params ["_objects", "_name", "_identifier", "_description", "_condition", "_function"];
private _identifierOutput = format ["KH_var_terminalOutput%1", _identifier];
private _identifierInput = format ["KH_var_terminalInput%1", _identifier];
missionNamespace setVariable [_identifierOutput, "", true];
missionNamespace setVariable [_identifierInput, "", true];
_identifier = format ["KH_var_terminal%1Accessed", _identifier];
missionNamespace setVariable [_identifier, false, true];

{
	[
		[
			_x,
			format ["Access Terminal %1", _name],
			"\a3\ui_f\data\IGUI\Cfg\holdactions\holdAction_hack_ca.paa",
			"\a3\ui_f\data\IGUI\Cfg\holdactions\holdAction_hack_ca.paa",
			"((_this distance _target) < 4)",
			["(!(missionNamespace getVariable ['", _identifier, "', false]) && ([] call ", _condition, ") && ((_caller distance _target) < 4))"] joinString "",
			{
				private _identifier = (_this select 3) select 1;
				private _condition = (_this select 3) select 7;

				if (missionNamespace getVariable [_identifier, false]) then {
					hint "Someone is already using a terminal with this identifier.";
				};

				if !([] call _condition) then {
					hint "I cannot access this terminal.";
				};
			},
			{},
			{
				(_this select 3) params ["_entity", "_identifier", "_identifierOutput", "_identifierInput", "_name", "_description", "_function"];

				[
					[_identifier],
					{
						params ["_identifier"];
						missionNamespace setVariable [_identifier, true, true];
					},
					"SERVER",
					"THIS_FRAME"
				] call KH_fnc_execute;

				[_entity, _identifier, _identifierOutput, _identifierInput, _name, _description, _function, false] call KH_fnc_openTerminal;
			},
			{},
			[_x, _identifier, _identifierOutput, _identifierInput, _name, _description, _function, _condition],
			1,
			0,
			false,
			false,
			true
		],
		"BIS_fnc_holdActionAdd",
		["JIP", "PLAYERS", _x, false, false, ""], 
		"THIS_FRAME"
	] call KH_fnc_execute;

	[
		[
			_x,
			format ["View Terminal %1", _name],
			"\a3\ui_f\data\IGUI\Cfg\holdactions\holdAction_hack_ca.paa",
			"\a3\ui_f\data\IGUI\Cfg\holdactions\holdAction_hack_ca.paa",
			"((_this distance _target) < 4)",
			"((_caller distance _target) < 4)",
			{},
			{},
			{
				(_this select 3) params ["_entity", "_identifier", "_identifierOutput", "_identifierInput", "_name", "_description", "_function"];
				[_entity, _identifier, _identifierOutput, _identifierInput, _name, _description, _function, true] call KH_fnc_openTerminal;
			},
			{},
			[_x, _identifier, _identifierOutput, _identifierInput, _name, _description, _function, _condition],
			1,
			0,
			false,
			false,
			true
		],
		"BIS_fnc_holdActionAdd",
		["JIP", "PLAYERS", _x, false, false, ""], 
		"THIS_FRAME"
	] call KH_fnc_execute;
} forEach _objects;

[_identifier, _identifierOutput];