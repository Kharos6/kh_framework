params ["_display", "_positionX", "_positionY", "_options"];

if !KH_var_contextMenuOpen then {
	KH_var_contextMenuOpen = true;
	KH_var_contextMenuValidOptions = [];
	KH_var_contextMenuInvalidOptions = [];
	KH_var_contextMenuControls = [];

	[
		{
			_args params ["_display", "_positionX", "_positionY", "_options"];

			if !(isNull _display) then {
				private _valid = [];
				private _invalid = [];

				{
					private _condition = _x select 2;

					if ([] call _condition) then {
						_valid pushBack _x;
					}
					else {
						_invalid pushBack _x;
					};
				} forEach _options;

				if ((_invalid isNotEqualTo KH_var_contextMenuInvalidOptions) || (_valid isNotEqualTo KH_var_contextMenuValidOptions)) then {
					KH_var_contextMenuValidOptions = [];
					KH_var_contextMenuInvalidOptions = [];
					KH_var_contextMenuControls = [];

					{
						_x params ["_name", "_tooltip", "_condition", "_arguments", "_function"];

						if ([] call _condition) then {
							private _control = _display ctrlCreate ["RscButton", -1, controlNull];
							KH_var_contextMenuControls pushBack _control;

							if ((count KH_var_contextMenuControls) > 1) then {
								_positionY = ((ctrlPosition (KH_var_contextMenuControls select ((KH_var_contextMenuControls find _control) - 1))) select 1) + 0.042;
							};

							_control ctrlSetPosition [_positionX, _positionY, 0.45, 0.04];
							_control ctrlSetText _name;
							_control ctrlSetTooltip _tooltip;
							_control ctrlCommit 0;

							[
								["CONTROL", _control],
								"ButtonClick",
								[_display, _condition, _arguments, _function],
								{
									_args params ["_display", "_condition", "_arguments", "_function"];

									if ([] call _condition) then {
										_arguments call _function;
									};
								}
							] call KH_fnc_addEventHandler;

							KH_var_contextMenuValidOptions pushBack _x;
						}
						else {
							KH_var_contextMenuInvalidOptions pushBack _x;
						};
					} forEach _options;
				};
			}
			else {
				[_handle] call CBA_fnc_removePerFrameHandler;
			};
		},
		0, 
		[_display, _positionX, _positionY, _options]
	] call CBA_fnc_addPerFrameHandler;

	[
		["DISPLAY", _display],
		"MouseButtonUp",
		[_display],
		{
			_args params ["_display"];

			{
				ctrlDelete _x;
			} forEach KH_var_contextMenuControls;
			
			KH_var_contextMenuOpen = false;
			_display displayRemoveEventHandler ["MouseButtonUp", _localId];
		}
	] call KH_fnc_addEventHandler;

	[
		["DISPLAY", _display],
		"KeyDown",
		[_display],
		{
			_args params ["_display"];
			
			{
				ctrlDelete _x;
			} forEach KH_var_contextMenuControls;
			
			KH_var_contextMenuOpen = false;
			_display displayRemoveEventHandler ["KeyDown", _localId];
			nil;
		}
	] call KH_fnc_addEventHandler;

	[
		{
			params ["_display"];
			(isNull _display);
		},
		{
			KH_var_contextMenuOpen = false;
		},
		[_display]
	] call CBA_fnc_waitUntilAndExecute;

	KH_var_contextMenuControls;
}
else {
	nil;
};