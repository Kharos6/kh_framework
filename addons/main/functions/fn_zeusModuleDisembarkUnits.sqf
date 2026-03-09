isNil {
	params [["_logic", objNull, [objNull]]];

	[
		[_logic],
		{
			params ["_logic"];

            {
				unassignVehicle _x;
                moveOut _x;
            } forEach (crew (attachedTo _logic));

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