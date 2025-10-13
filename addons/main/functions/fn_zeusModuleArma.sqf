isNil {
	params [["_logic", objNull, [objNull]]];
	[attachedTo _logic] call KH_fnc_arma;
	deleteVehicle _logic;
};

nil;