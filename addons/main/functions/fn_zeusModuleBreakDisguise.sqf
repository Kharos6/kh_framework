isNil {
	params [["_logic", objNull, [objNull]]];
	[attachedTo _logic] call KH_fnc_breakDisguise;
	deleteVehicle _logic;
};

nil;