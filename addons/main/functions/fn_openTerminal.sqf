params ["_entity", "_identifier", "_identifierOutput", "_name", "_description", "_function"];
private _display = createDialog ["KH_ResourceTerminal", true];
ctrlSetText [100, _name];
ctrlSetText [101, _description];
ctrlSetText [102, missionNamespace getVariable [_identifierOutput, "..."]];
player setVariable ["KH_var_dialogActive", true, 2];

[
	["CONTROL", _display displayCtrl 104],
	"ButtonClick",
	[_display, _identifierOutput, _function, _display],
	{
		_args params ["_display", "_identifierOutput", "_function", "_display"];
		private _input = ctrlText 103;
		private _command = _input select [0, (_input find "-") - 1];
		private _argument = _input select [(_input find "-") + 1];
		private _functionOutput = [_command, _argument] call _function;

		if !(_functionOutput isEqualType "") then {
			_functionOutput = "EXECUTION FAILED";
		};

		private _output = [missionNamespace getVariable [_identifierOutput, ctrlText 102], _functionOutput] joinString "\n";

		[
			[_output],
			{
				params ["_output"];
				missionNamespace setVariable [_identifierOutput, _output, true];
			},
			"SERVER",
			"THIS_FRAME"
		] call KH_fnc_execute;

		ctrlSetText [102, _output];
	}
] call KH_fnc_addEventHandler;

[
	["CONTROL", _display displayCtrl 2],
	"ButtonClick",
	[_identifier],
	{
		params ["_control"];
		_args params ["_identifier"];

		[
			[_identifier],
			{
				params ["_identifier"];
				missionNamespace setVariable [_identifier, false, true];
			},
			"SERVER",
			"THIS_FRAME"
		] call KH_fnc_execute;
		
		player setVariable ["KH_var_dialogActive", false, 2];
		_control ctrlRemoveEventHandler ["ButtonClick", _localId];
	}
] call KH_fnc_addEventHandler;

[
	{
		!dialog;
	}, 
	{
		params ["_identifier"];

		[
			[_identifier],
			{
				params ["_identifier"];
				missionNamespace setVariable [_identifier, false, true];
			},
			"SERVER",
			"THIS_FRAME"
		] call KH_fnc_execute;

		player setVariable ["KH_var_dialogActive", false, 2];
	}, 
	[_identifier]
] call CBA_fnc_waitUntilAndExecute;

[
	[player, _entity, _identifier],
	{
		params ["_player", "_entity", "_identifier"];

		[
			{
				params ["_player", "_entity", "_identifier"];
				(!(alive _player) || !(alive _entity) || ((_entity distance _player) > 4) || (isNull _entity) || (isNull _player));
			}, 
			{
				private _player = _this select 0;
				private _identifier = _this select 2;
				missionNamespace setVariable [_identifier, false, true];

				[
					[_player],
					{
						params ["_player"];

						if (_player getVariable ["KH_var_dialogActive", false]) then {
							closeDialog 0;
						};
					},
					_player,
					"THIS_FRAME"
				] call KH_fnc_execute;
			}, 
			[_player, _entity]
		] call CBA_fnc_waitUntilAndExecute;
	},
	"SERVER",
	"THIS_FRAME"
] call KH_fnc_execute;