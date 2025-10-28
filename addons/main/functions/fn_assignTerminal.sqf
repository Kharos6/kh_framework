params [["_object", objNull, [objNull]], ["_name", "", [""]], ["_identifier", "", [""]], ["_description", "", [""]], ["_condition", {true;}, [{}]], ["_function", {}, [{}]]];
private _identifierOutput = ["KH_var_terminalOutput_", _identifier] joinString "";
private _identifierInput = ["KH_var_terminalInput_", _identifier] joinString "";
missionNamespace setVariable [_identifierOutput, "", true];
missionNamespace setVariable [_identifierInput, "", true];
_identifier = ["KH_var_terminalAccessed_", _identifier] joinString "";
missionNamespace setVariable [_identifier, false, true];

[
	[
		_object,
		["Access ", _name] joinString "",
		"\a3\ui_f\data\IGUI\Cfg\holdactions\holdAction_hack_ca.paa",
		"\a3\ui_f\data\IGUI\Cfg\holdactions\holdAction_hack_ca.paa",
		"((_this distance _target) < 4)",
		["(!(missionNamespace getVariable '", _identifier, "') && (call ", _condition, ") && ((_caller distance _target) < 4))"] joinString "",
		{
			private _args = param [3];
			private _identifier = _args param [1];
			private _condition = _args param [7];

			if (missionNamespace getVariable _identifier) then {
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

			[_entity, _identifier, _identifierOutput, _identifierInput, _name, _description, _function] call KH_fnc_openTerminal;
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