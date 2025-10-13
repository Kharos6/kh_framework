params [["_object", objNull, [objNull]], ["_name", "", [""]], ["_identifier", "", [""]], ["_description", "", [""]], ["_condition", {}, [{}]], ["_function", {}, [{}]]];
private _identifierOutput = format ["KH_var_terminalOutput%1", _identifier];
private _identifierInput = format ["KH_var_terminalInput%1", _identifier];
missionNamespace setVariable [_identifierOutput, "", true];
missionNamespace setVariable [_identifierInput, "", true];
_identifier = format ["KH_var_terminal%1Accessed", _identifier];
missionNamespace setVariable [_identifier, false, true];

[
	[
		_object,
		format ["Access Terminal %1", _name],
		"\a3\ui_f\data\IGUI\Cfg\holdactions\holdAction_hack_ca.paa",
		"\a3\ui_f\data\IGUI\Cfg\holdactions\holdAction_hack_ca.paa",
		"((_this distance _target) < 4)",
		["(!(missionNamespace getVariable ['", _identifier, "', false]) && (call ", _condition, ") && ((_caller distance _target) < 4))"] joinString "",
		{
			private _args = param [3];
			private _identifier = _args param [1];
			private _condition = _args param [7];

			if (missionNamespace getVariable [_identifier, false]) then {
				hint "Someone is already using this terminal.";
			};

			if !(call _condition) then {
				hint "I cannot access this terminal.";
			};
		},
		{},
		{
			private _args = param [3];
			_args params ["_entity", "_identifier", "_identifierOutput", "_identifierInput", "_name", "_description", "_function"];

			[
				[_identifier],
				{
					params ["_identifier"];
					missionNamespace setVariable [_identifier, true, true];
				},
				"SERVER",
				true,
				false
			] call KH_fnc_execute;

			[_entity, _identifier, _identifierOutput, _identifierInput, _name, _description, _function, false] call KH_fnc_openTerminal;
		},
		{},
		[_object, _identifier, _identifierOutput, _identifierInput, _name, _description, _function, _condition],
		1,
		0,
		false,
		false,
		true
	],
	"BIS_fnc_holdActionAdd",
	"PLAYERS",
	true,
	["JIP", _object, false, ""]
] call KH_fnc_execute;

[
	[
		_object,
		format ["View Terminal %1", _name],
		"\a3\ui_f\data\IGUI\Cfg\holdactions\holdAction_hack_ca.paa",
		"\a3\ui_f\data\IGUI\Cfg\holdactions\holdAction_hack_ca.paa",
		"((_this distance _target) < 4)",
		"((_caller distance _target) < 4)",
		{},
		{},
		{
			private _args = param [3];
			_args params ["_entity", "_identifier", "_identifierOutput", "_identifierInput", "_name", "_description", "_function"];
			[_entity, _identifier, _identifierOutput, _identifierInput, _name, _description, _function, true] call KH_fnc_openTerminal;
		},
		{},
		[_object, _identifier, _identifierOutput, _identifierInput, _name, _description, _function, _condition],
		1,
		0,
		false,
		false,
		true
	],
	"BIS_fnc_holdActionAdd",
	"PLAYERS",
	true,
	["JIP", _object, false, ""]
] call KH_fnc_execute;

[_identifier, _identifierOutput];