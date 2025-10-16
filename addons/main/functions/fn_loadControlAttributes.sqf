params [["_controlGroup", controlNull, [controlNull]], ["_value", [], [[]]]];
private _count = (count _value) - 1;
private _idcOffset = 100;

for "_i" from 0 to _count do {
	private _currentControl = _controlGroup controlsGroupCtrl _idcOffset;
	private _type = ctrlType _currentControl;
	
	switch true do {
		case (_type isEqualTo 77): {
			_currentControl cbSetChecked (_value select _i);
		};
		
		case (_type isEqualTo 2): {
			_currentControl ctrlSetText (_value select _i);
		};
		
		case ((_type isEqualTo 4) || ((_type isEqualTo 5) && ((ctrlStyle _currentControl) isNotEqualTo 32)) || (_type isEqualTo 6) || (_type isEqualTo 44) || (_type isEqualTo 45) || (_type isEqualTo 102)): {
			_currentControl lbSetCurSel (_value select _i);
		};

		case ((_type isEqualTo 5) && ((ctrlStyle _currentControl) isEqualTo 32)): {
			_currentControl lbSetSelected [_value select _i, true];
		};
		
		case (_type isEqualTo 7): {
			private _count = (count (_value select _i)) - 1;

			for "_j" from 0 to _count do {
				_currentControl ctrlSetChecked [((_value select _i) select _j) select 0, ((_value select _i) select _j) select 1];
			};
		};
		
		case (_type isEqualTo 12): {
			_currentControl tvSetCurSel (_value select _i);
		};
		
		case ((_type isEqualTo 3) || (_type isEqualTo 43)): {
			_currentControl sliderSetPosition (_value select _i);
		};
	};
	
	_idcOffset = _idcOffset + 1;
};

nil;