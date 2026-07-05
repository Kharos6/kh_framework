params [
	["_entity", objNull, [objNull]], 
	["_identifier", "", [""]], 
	["_identifierOutput", "", [""]],
	["_identifierInput", "", [""]], 
	["_name", "", [""]], 
	["_description", "", [""]], 
	["_function", {}, [{}]]
];

private _display = createDialog ["KH_ResourceTerminal", true];
ctrlSetText [100, toUpperANSI _name];
ctrlSetText [101, _description];
ctrlSetText [102, missionNamespace getVariable [_identifierOutput, ctrlText 102]];
ctrlSetText [103, missionNamespace getVariable [_identifierInput, ctrlText 103]];

execute [
	[_identifierOutput, _identifierInput],
	{
		params ["_identifierOutput", "_identifierInput"];

		if !dialog then {
			[_handlerId] call KH_fnc_removeHandler;
		}
		else {
			ctrlSetText [102, missionNamespace getVariable [_identifierOutput, ctrlText 102]];
			private _input = ctrlText 103;

			execute [
				[_identifierInput, _input],
				{
					params ["_identifierInput", "_input"];
					missionNamespace setVariable [_identifierInput, _input, true];
				},
				"SERVER",
				true,
				false
			];
		};
	},
	true,
	0,
	false
];

[
	["CONTROL", _display displayCtrl 104],
	"ButtonClick",
	[_identifierOutput, _function],
	{
		_args params ["_identifierOutput", "_function"];

		execute [
			[_identifierOutput, _function, ctrlText 103],
			{
				params ["_identifierOutput", "_function", "_input"];
				private _functionOutput = [_input, _identifierOutput] call _function;

				if (_functionOutput isEqualType "") then {
					private _output = [missionNamespace getVariable [_identifierOutput, ""], _functionOutput] joinString "";
					missionNamespace setVariable [_identifierOutput, _output, true];
				};
			},
			"SERVER",
			true,
			false
		];

		ctrlSetText [103, ""];
		nil;
	}
] call KH_fnc_addEventHandler;

[
	["CONTROL", _display displayCtrl 2],
	"ButtonClick",
	[_identifier],
	{
		params ["_control"];
		_args params ["_identifier"];

		execute [
			[_identifier],
			{
				params ["_identifier"];
				missionNamespace setVariable [_identifier, false, true];
			},
			"SERVER",
			true,
			false
		];
		
		[_handlerId] call KH_fnc_removeHandler;
		nil;
	}
] call KH_fnc_addEventHandler;

[
	["DISPLAY", _display],
	"KeyDown",
	[_identifierOutput, _function],
	{
		(_this select [1]) params ["_key", "_shift"];
		_args params ["_identifierOutput", "_function"];

		if ((_key isEqualTo 0x1C) && !_shift) then {
			execute [
				[_identifierOutput, _function, ctrlText 103],
				{
					params ["_identifierOutput", "_function", "_input"];
					private _functionOutput = [_input, _identifierOutput] call _function;

					if (_functionOutput isEqualType "") then {
						private _output = [missionNamespace getVariable [_identifierOutput, ""], _functionOutput] joinString "";
						missionNamespace setVariable [_identifierOutput, _output, true];
					};
				},
				"SERVER",
				true,
				false
			];

			ctrlSetText [103, ""];
			true;
		};
	}
] call KH_fnc_addEventHandler;

execute [
	[player, _entity, _identifier],
	{
		params ["_player", "_entity", "_identifier"];

		execute [
			[_player, _entity, _identifier], 
			{
				private _player = param [0];
				private _identifier = param [2];
				missionNamespace setVariable [_identifier, false, true];

				execute [
					[],
					{
						if dialog then {
							closeDialog 0;
						};
					},
					_player,
					true,
					false
				];
			},
			true,
			{
				params ["_player", "_entity", "_identifier"];
				(!(alive _player) || !(alive _entity) || ((_entity distance _player) > 4) || (isNull _entity) || (isNull _player));
			},
			false
		];
	},
	"SERVER",
	true,
	false
];

execute [
	[_identifier],
	{
		params ["_identifier"];
		missionNamespace setVariable [_identifier, false, true];
	},
	true,
	{!dialog},
	false
];

_display;