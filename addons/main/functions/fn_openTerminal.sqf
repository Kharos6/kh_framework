params ["_entity", "_identifier", "_identifierOutput", "_name", "_description", "_function"];
private _display = createDialog ["KH_ResourceTerminal", true];
ctrlSetText [100, _name];
ctrlSetText [101, _description];
ctrlSetText [102, missionNamespace getVariable [_identifierOutput, ctrlText 102]];
player setVariable ["KH_var_dialogActive", true];

[
	{
		_args params ["_identifier", "_identifierOutput"];

		if !dialog then {
			[_handle] call CBA_fnc_removePerFrameHandler;
			player setVariable ["KH_var_dialogActive", false];
		}
		else {
			ctrlSetText [102, missionNamespace getVariable [_identifierOutput, ctrlText 102]];
		};
	},
	0, 
	[_identifier, _identifierOutput]
] call CBA_fnc_addPerFrameHandler;

[
	["DISPLAY", _display],
	"KeyUp",
	[_identifierOutput, _function],
	{
		private _key = _this select 1;

		if (_key isEqualTo 0x1C) then {
			_args params ["_identifierOutput", "_function"];
			private _input = ctrlText 103;
			private _command = "";
			private _argument = "";

			if ("-" in _input) then {
				_command = _input select [0, (_input find "-") - 1];
				_argument = _input select [(_input find "-") + 1];
			}
			else {
				_command = _input;
			};

			[
				[_identifierOutput, _function, _command, _argument],
				{
					params ["_identifierOutput", "_function", "_command", "_argument"];
					private _functionOutput = [_command, _argument, _identifierOutput] call _function;

					if (_functionOutput isEqualType "") then {
						private _output = [missionNamespace getVariable [_identifierOutput, ""], _functionOutput] joinString "";
						missionNamespace setVariable [_identifierOutput, _output, true];
					};
				},
				"SERVER",
				"THIS_FRAME"
			] call KH_fnc_execute;

			ctrlSetText [103, ""];
		};
	}
] call KH_fnc_addEventHandler;

[
	["CONTROL", _display displayCtrl 104],
	"ButtonClick",
	[_identifierOutput, _function],
	{
		_args params ["_identifierOutput", "_function"];
		private _input = ctrlText 103;
		private _command = "";
		private _argument = "";

		if ("-" in _input) then {
			_command = _input select [0, (_input find "-") - 1];
			_argument = _input select [(_input find "-") + 1];
		}
		else {
			_command = _input;
		};

		[
			[_identifierOutput, _function, _command, _argument],
			{
				params ["_identifierOutput", "_function", "_command", "_argument"];
				private _functionOutput = [_command, _argument, _identifierOutput] call _function;

				if (_functionOutput isEqualType "") then {
					private _output = [missionNamespace getVariable [_identifierOutput, ""], _functionOutput] joinString "";
					missionNamespace setVariable [_identifierOutput, _output, true];
				};
			},
			"SERVER",
			"THIS_FRAME"
		] call KH_fnc_execute;

		ctrlSetText [103, ""];
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
		
		player setVariable ["KH_var_dialogActive", false];
		_control ctrlRemoveEventHandler ["ButtonClick", _localId];
	}
] call KH_fnc_addEventHandler;

[
	[player, _entity, _identifier],
	{
		params ["_player", "_entity", "_identifier"];

		[
			{
				params ["_player", "_entity", "_identifier"];
				(!(alive _player) || !(alive _entity) || ((_entity distance _player) > 4) || (isNull _entity) || (isNull _player) || !(_player getVariable ["KH_var_dialogActive", false]));
			}, 
			{
				private _player = _this select 0;
				private _identifier = _this select 2;
				missionNamespace setVariable [_identifier, false, true];

				[
					[],
					{
						if (player getVariable ["KH_var_dialogActive", false]) then {
							closeDialog 0;
						};
					},
					_player,
					"THIS_FRAME"
				] call KH_fnc_execute;
			}, 
			[_player, _entity, _identifier]
		] call CBA_fnc_waitUntilAndExecute;
	},
	"SERVER",
	"THIS_FRAME"
] call KH_fnc_execute;

_display;