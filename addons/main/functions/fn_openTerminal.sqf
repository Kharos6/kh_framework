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

[
	[_identifierOutput, _identifierInput],
	{
		params ["_identifierOutput", "_identifierInput"];

		if !dialog then {
			[_handlerId] call KH_fnc_removeHandler;
		}
		else {
			ctrlSetText [102, missionNamespace getVariable [_identifierOutput, ctrlText 102]];
			private _input = ctrlText 103;

			[
				[_identifierInput, _input],
				{
					params ["_identifierInput", "_input"];
					missionNamespace setVariable [_identifierInput, _input, true];
				},
				"SERVER",
				true,
				false
			] call KH_fnc_execute;
		};
	},
	true,
	0,
	false
] call KH_fnc_execute;

[
	["CONTROL", _display displayCtrl 104],
	"ButtonClick",
	[_identifierOutput, _function],
	{
		_args params ["_identifierOutput", "_function"];

		[
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
		] call KH_fnc_execute;

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

		[
			[_identifier],
			{
				params ["_identifier"];
				missionNamespace setVariable [_identifier, false, true];
			},
			"SERVER",
			true,
			false
		] call KH_fnc_execute;
		
		[_handlerId] call KH_fnc_removeHandler;
		nil;
	}
] call KH_fnc_addEventHandler;

[
	["DISPLAY", _display],
	"KeyDown",
	[_identifierOutput, _function],
	{
		params ["_display", "_key", "_shift"];
		_args params ["_identifierOutput", "_function"];

		if ((_key isEqualTo 0x1C) && !_shift) then {
			[
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
			] call KH_fnc_execute;

			ctrlSetText [103, ""];
			true;
		};
	}
] call KH_fnc_addEventHandler;

[
	[player, _entity, _identifier],
	{
		params ["_player", "_entity", "_identifier"];

		[
			[_player, _entity, _identifier], 
			{
				private _player = param [0];
				private _identifier = param [2];
				missionNamespace setVariable [_identifier, false, true];

				[
					[],
					{
						if dialog then {
							closeDialog 0;
						};
					},
					_player,
					true,
					false
				] call KH_fnc_execute;
			},
			true,
			{
				params ["_player", "_entity", "_identifier"];
				(!(alive _player) || !(alive _entity) || ((_entity distance _player) > 4) || (isNull _entity) || (isNull _player));
			},
			false
		] call KH_fnc_execute;
	},
	"SERVER",
	true,
	false
] call KH_fnc_execute;

[
	[_identifier],
	{
		params ["_identifier"];
		missionNamespace setVariable [_identifier, false, true];
	},
	true,
	{!dialog},
	false
] call KH_fnc_execute;

_display;