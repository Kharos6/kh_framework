params ["_controlGroup", "_value"];
private _count = (count _value) - 1;
private _idcOffset = 100;
private _i = 0;

for "_i" from 0 to _count do {
	private _currentControl = _controlGroup controlsGroupCtrl _idcOffset;
	private _type = ctrlType _currentControl;
	
	switch true do {
		case (_type == 77): {
			_currentControl cbSetChecked (_value select _i);
		};
		
		case (_type == 2): {
			_currentControl ctrlSetText (_value select _i);
		};
		
		case ((_type == 4) || ((_type == 5) && ((ctrlStyle _currentControl) != 32)) || (_type == 6) || (_type == 44) || (_type == 45) || (_type == 102)): {
			_currentControl lbSetCurSel (_value select _i);
		};

		case ((_type == 5) && ((ctrlStyle _currentControl) == 32)): {
			_currentControl lbSetSelected [_value select _i, true];
		};
		
		case (_type == 7): {
			_currentControl ctrlSetChecked (_value select _i);
		};
		
		case (_type == 12): {
			_currentControl tvSetCurSel (_value select _i);
		};
		
		case ((_type == 3) || (_type == 43)): {
			_currentControl sliderSetPosition (_value select _i);
		};
	};
	
	_idcOffset = _idcOffset + 1;
};

true;