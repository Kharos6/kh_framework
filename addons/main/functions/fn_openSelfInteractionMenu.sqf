private _display = (findDisplay 46) createDisplay "KH_ResourceSelfInteractionMenu";
KH_var_currentInteractionTarget = player;
KH_var_validSelfInteractionEntries = [];
KH_var_validSelfInteractionOptions = [];
private _list = player getVariable ["KH_var_selfInteractionList", []]; 
private _count = (count _list) - 1;
private _i = 0;
private _mainControl = _display displayCtrl 100;

for "_i" from 0 to _count do {
	(_list select _i) params ["_name", "_condition", "_options"];

	if ([] call _condition) then {
		KH_var_validSelfInteractionEntries pushBack (_list select _i);
		KH_var_validSelfInteractionOptions pushBack _options;
		_mainControl lbAdd _name;
	};
};

[
	{
		_args params ["_display"];

		if !(isNull _display) then {
			private _currentEntries = [];
			private _currentOptions = [];
			private _list = player getVariable ["KH_var_selfInteractionList", []]; 
			private _count = (count _list) - 1;
			private _i = 0;
			private _mainControl = _display displayCtrl 100;

			for "_i" from 0 to _count do {
				(_list select _i) params ["_name", "_condition", "_options"];

				if ([] call _condition) then {
					_currentEntries pushBack (_list select _i);
					_currentOptions pushBack _options;
				};
			};

			if (_currentEntries isNotEqualTo KH_var_validSelfInteractionEntries) then {
				KH_var_validSelfInteractionEntries = _currentEntries;
				KH_var_validSelfInteractionOptions = _currentOptions;
				lbClear _mainControl;

				{
					_mainControl lbAdd (_x select 0);
				} forEach _currentEntries;
			};
		}
		else {
			[_handle] call CBA_fnc_removePerFrameHandler;
		};
	},
	0, 
	[_display]
] call CBA_fnc_addPerFrameHandler;

[
	["CONTROL", _mainControl],
	"MouseButtonClick",
	[_display, _mainControl],
	{
		_args params ["_display", "_mainControl"];
		private _positionX = ((ctrlPosition _mainControl) select 0) + 0.64;
		private _positionY = (ctrlPosition _mainControl) select 1;

		if (lbSize _mainControl != 0) then {
			private _selection = lbCurSel _mainControl;
			[_display, _positionX, _positionY, KH_var_validSelfInteractionOptions select _selection] call KH_fnc_contextMenu;
		};
	}
] call KH_fnc_addEventHandler;

[
	["CONTROL", _mainControl],
	"KeyUp",
	[_display, _mainControl],
	{
		_args params ["_display", "_mainControl"];
		private _key = _this select 1;

		if ((_key isEqualTo 0xD0) || (_key isEqualTo 0xC8) || (_key isEqualTo 0x1C)) then {
			private _positionX = ((ctrlPosition _mainControl) select 0) + 0.64;
			private _positionY = (ctrlPosition _mainControl) select 1;

			if (lbSize _mainControl != 0) then {
				private _selection = lbCurSel _mainControl;
				[_display, _positionX, _positionY, KH_var_validSelfInteractionOptions select _selection] call KH_fnc_contextMenu;
			};
		};
	}
] call KH_fnc_addEventHandler;

[
	["DISPLAY", _display],
	"KeyUp",
	[_display],
	{
		_args params ["_display"];
		private _key = _this select 1;

		if (_key isEqualTo 0xDC) then {
			_display closeDisplay 2;
		};
	}
] call KH_fnc_addEventHandler;

_display;