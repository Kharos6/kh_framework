params ["_controlGroup", ["_config", configNull]];
private _controlsIDCSorted = [];
private _attributes = [];

{
	private _currentIDC = ctrlIDC _x;
	
	if ((_currentIDC != -1) && (_currentIDC >= 100)) then {
		_controlsIDCSorted pushBack _currentIDC;
	};
} forEach (allControls _controlGroup);

_controlsIDCSorted sort true;

{
	private _returnValue = objNull;
	private _currentControl = _controlGroup controlsGroupCtrl _x;
 	private _type = ctrlType _currentControl;
	
	switch true do {		
		case (_type == 77): {
			_returnValue = cbChecked _currentControl;
		};
		
		case (_type == 2): {
			_returnValue = ctrlText _currentControl;
		};
		
		case ((_type == 4) || ((_type == 5) && ((ctrlStyle _currentControl) != 32)) || (_type == 6) || (_type == 44) || (_type == 45) || (_type == 102)): {
			_returnValue = lbCurSel _currentControl;
		};

		case ((_type == 5) && ((ctrlStyle _currentControl) == 32)): {
			_returnValue = lbSelection _currentControl;
		};
		
		case (_type == 7): {
			private _checkboxes = [];
			private _i = 0;
			private _amount = (count (getArray _config)) - 1;

			for "_i" from 0 to _amount do {
				_checkboxes pushBack [_i, (_currentControl ctrlChecked _i)];
			};

			_returnValue = _checkboxes;
		};
		
		case (_type == 12): {
			_returnValue = tvCurSel _currentControl;
		};
		
		case ((_type == 3) || (_type == 43)): {
			_returnValue = sliderPosition _currentControl;
		};
	};
	
	_attributes pushBack _returnValue;
} forEach _controlsIDCSorted;

_attributes;