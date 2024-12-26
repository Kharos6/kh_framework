private _display = (findDisplay 46) createDisplay "KH_ResourceSelfInteractionMenu";
private _list = player getVariable ["KH_var_selfInteractionList", []]; 
private _count = (count _list) - 1;
private _i = 0;
private _validOptions = [];
private _mainControl = _display displayCtrl 100;

for "_i" from 0 to _count do {
	(_list select _i) params ["_name", "_condition", "_options"];

	if ([] call _condition) then {
		_validOptions pushBack _options;
		_mainControl lbAdd _name;
	};
};

[
	["CONTROL", _mainControl],
	"MouseButtonClick",
	[_display, _validOptions, _mainControl],
	{
		_args params ["_display", "_validOptions", "_mainControl"];
		private _positionX = ((ctrlPosition _mainControl) select 0) + 2;
		private _positionY = (ctrlPosition _mainControl) select 1;
		private _selection = lbCurSel _mainControl;
		[_display, _positionX, _positionY, _validOptions select _selection] call KH_fnc_contextMenu;
	}
] call KH_fnc_addEventHandler;

_display;