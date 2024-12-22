isNil {
	params [["_logic", objNull]];
	private _entity = attachedTo _logic;
	deleteVehicle _logic;

	if (_entity isKindOf "Man") then {
		_entity addForce [[0, 0, 1], [0, 0, 0], false];
	};
};

true;