isNil {
	params [["_logic", objNull, [objNull]]];

	[
		[_logic],
		{
			params ["_logic"];
			private _unit = attachedTo _logic;

			[
				[_unit],
				{
					params ["_unit"];
					_unit addForce [[0, 0, 1], [0, 0, 0], false];
				},
				_unit,
				true,
				false
			] call KH_fnc_execute;

			deleteVehicle _logic;
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