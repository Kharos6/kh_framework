isNil {
	params [["_logic", objNull, [objNull]]];

	[
		[_logic],
		{
			params ["_logic"];
			(attachedTo _logic) setDamage 1;
			deleteVehicle _logic;
			[_handlerId] call KH_fnc_removeHandler;
		},
		true,
		[
			{
				params ["_logic"];
				!(isNull (attachedTo _logic));
			},
			true,
			0,
			1,
			{},
			false,
			true
		],
		false
	] call KH_fnc_execute;
};

nil;