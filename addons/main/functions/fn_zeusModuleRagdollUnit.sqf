isNil {
	params [["_logic", objNull, [objNull]]];

	execute [
		[_logic],
		{
			params ["_logic"];
			private _unit = attachedTo _logic;

			execute [
				[_unit],
				{
					params ["_unit"];
					_unit addForce [[0, 0, 1], [0, 0, 0], false];
				},
				_unit,
				true,
				false
			];

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
	];
};

nil;