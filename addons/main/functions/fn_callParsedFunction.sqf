params ["_arguments", ["_function", "", [""]], ["_caller", 2, [0]]];
private _storedFunction = missionNamespace getVariable _function;

if !(isNil "_storedFunction") exitWith {
	_arguments call _storedFunction;
};

[
	[_arguments, _function, _caller],
	{
		params ["_arguments", "_function", "_caller"];
		_argsCallback params ["_storedFunction"];

		if !(isNil "_storedFunction") exitWith {
			missionNamespace setVariable [_function, _storedFunction];
			_arguments call _storedFunction;
		};
		
		if ((_caller isEqualTo 2) || (_caller isEqualTo clientOwner)) exitWith {};
		
		[
			[_arguments, _function],
			{
				params ["_arguments", "_function"];
				_argsCallback params ["_storedFunction"];

				if !(isNil "_storedFunction") then { 
					missionNamespace setVariable [_function, _storedFunction];

					if !isServer then {
						missionNamespace setVariable [_function, _storedFunction, 2];
					};

					_arguments call _storedFunction;
				};
			},
			_caller,
			true,
			[
				"CALLBACK",
				[_function],
				{
					params ["_function"];
					private _storedFunction = missionNamespace getVariable _function;
					
					if !(isNil "_storedFunction") then {
						[_storedFunction];
					}
					else {
						[];
					};											
				}
			]
		] call KH_fnc_execute;
	},
	"SERVER",
	true,
	[
		"CALLBACK",
		[_function],
		{
			params ["_function"];
			private _storedFunction = missionNamespace getVariable _function;
			
			if !(isNil "_storedFunction") then {
				[_storedFunction];
			}
			else {
				[];
			};											
		}
	]
] call KH_fnc_execute;