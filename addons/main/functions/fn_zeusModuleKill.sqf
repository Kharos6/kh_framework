isNil {
	params [["_logic", objNull, [objNull]]];
    (attachedTo _logic) setDamage 1;
	deleteVehicle _logic;
};

nil;