params [["_controlGroup", controlNull, [controlNull]], ["_config", configNull, [configNull]]];
private _controlsIDCSorted = [];
private _attributes = [];

{
	private _currentIDC = ctrlIDC _x;
	
	if ((_currentIDC isNotEqualTo -1) && (_currentIDC >= 100)) then {
		_controlsIDCSorted pushBack _currentIDC;
	};
} forEach (allControls _controlGroup);

_controlsIDCSorted sort true;

{
	private _returnValue = objNull;
	private _currentControl = _controlGroup controlsGroupCtrl _x;
 	private _type = ctrlType _currentControl;
	
	switch true do {		
		case (_type isEqualTo 77): {
			_returnValue = cbChecked _currentControl;
		};
		
		case (_type isEqualTo 2): {
			_returnValue = ctrlText _currentControl;
		};
		
		case ((_type isEqualTo 4) || ((_type isEqualTo 5) && ((ctrlStyle _currentControl) isNotEqualTo 32)) || (_type isEqualTo 6) || (_type isEqualTo 44) || (_type isEqualTo 45) || (_type isEqualTo 102)): {
			_returnValue = lbCurSel _currentControl;
		};

		case ((_type isEqualTo 5) && ((ctrlStyle _currentControl) isEqualTo 32)): {
			_returnValue = lbSelection _currentControl;
		};
		
		case (_type isEqualTo 7): {
			private _checkboxes = [];
			private _amount = (count (getArray _config)) - 1;

			for "_i" from 0 to _amount do {
				_checkboxes pushBack [_i, (_currentControl ctrlChecked _i)];
			};

			_returnValue = _checkboxes;
		};
		
		case (_type isEqualTo 12): {
			_returnValue = tvCurSel _currentControl;
		};
		
		case ((_type isEqualTo 3) || (_type isEqualTo 43)): {
			_returnValue = sliderPosition _currentControl;
		};
	};
	
	_attributes pushBack _returnValue;
} forEach _controlsIDCSorted;

_attributes;