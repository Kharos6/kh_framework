isNil {
	params [["_logic", objNull]];
	deleteVehicle _logic;

	[
		[attachedTo _logic],
		{
			params ["_entity"];
			_entity addForce [[0, 0, 1], [0, 0, 0], false];
		},
		_entity,
		true,
		false
	] call KH_fnc_execute;
};