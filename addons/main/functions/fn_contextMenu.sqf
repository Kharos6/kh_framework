params ["_display", "_positionX", "_positionY", "_options"];

if !KH_var_contextMenuOpen then {
	KH_var_contextMenuOpen = true;
	private _controls = [];

	{
		_x params ["_name", "_tooltip", "_condition", "_arguments", "_function"];

		if ([] call _condition) then {
			private _control = _display ctrlCreate ["RscButton", -1, controlNull];
			_controls pushBack _control;

			if ((count _controls) > 1) then {
				_positionY = ((ctrlPosition (_controls select ((_controls find _control) - 1))) select 1) + 0.042;
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
		};
	} forEach _options;

	[
		["DISPLAY", _display],
		"MouseButtonUp",
		[_controls],
		{
			_args params ["_controls"];

			{
				ctrlDelete _x;
			} forEach _controls;
			
			KH_var_contextMenuOpen = false;
			_display displayRemoveEventHandler ["MouseButtonUp", _localId];
		}
	] call KH_fnc_addEventHandler;

	[
		["DISPLAY", _display],
		"KeyDown",
		[_controls],
		{
			_args params ["_controls"];
			
			{
				ctrlDelete _x;
			} forEach _controls;
			
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

	_controls;
}
else {
	nil;
};