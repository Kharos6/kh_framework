params ["_display", "_positionX", "_positionY", "_options"];
private _offset = 0;
private _controls = [];

{
	_x params ["_name", "_arguments", "_function"];
	private _control = _display ctrlCreate ["RscButton", -1, controlNull];
	_control ctrlSetPosition [_positionX, _positionY + _offset];
	_control ctrlSetText _name;
	_control ctrlCommit 0;

	[
		["CONTROL", _control],
		"ButtonClick",
		[_display, _arguments, _function],
		{
			_args params ["_display", "_arguments", "_function"];
			_arguments call _function;
			_display closeDisplay 2;
		}
	] call KH_fnc_addEventHandler;

	_offset = _offset + 1;
	_controls pushBack _control;
} forEach _options;

_controls;