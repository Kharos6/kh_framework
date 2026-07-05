params [["_vehicle", objNull, [objNull]]];

execute [
	[_vehicle],
	{
		params ["_vehicle"];
		_vehicle setAngularVelocity [(random 100) max 30, (random 100) max 30, (random 100) max 30];
	},
	_vehicle,
	true,
	false
];