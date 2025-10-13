isNil {
	params [["_logic", objNull, [objNull]]];

	[
		[attachedTo _logic],
		{
			params ["_unit"];
			_unit addForce [[0, 0, 1], [0, 0, 0], false];
		},
		_unit,
		true,
		false
	] call KH_fnc_execute;

	deleteVehicle _logic;
};

nil;