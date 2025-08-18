params ["_arguments", ["_function", "", [""]]];
private _storedFunction = missionNamespace getVariable _function;

if !(isNil "_storedFunction") exitWith {
	_arguments call _storedFunction;
};

private _caller = param [2, 2, [0]];

[
	[_arguments, _function, _caller],
	{
		params ["_arguments", "_function", "_caller"];
		_argsCallback params ["_storedFunction"];

		if !(isNil "_storedFunction") exitWith {
			missionNamespace setVariable [_function, _storedFunction];
			_arguments call _storedFunction;
		};

		[
			[_arguments, _function],
			{
				params ["_arguments", "_function"];
				_argsCallback params ["_storedFunction"];

				if !(isNil "_storedFunction") then { 
					missionNamespace setVariable [_function, _storedFunction];
					_arguments call _storedFunction;
				};
			},
			[
				"CALLBACK", 
				_caller, 
				[_function],
				{
					params ["_function"];
					private _storedFunction = missionNamespace getVariable _function;
					
					if !(isNil "_storedFunction") then {
						[_storedFunction];
					}
					else {
						[nil];
					};											
				}
			],
			true,
			false
		] call KH_fnc_execute;
	},
	[
		"CALLBACK", 
		"SERVER", 
		[_function],
		{
			params ["_function"];
			private _storedFunction = missionNamespace getVariable _function;
			
			if !(isNil "_storedFunction") then {
				[_storedFunction];
			}
			else {
				[nil];
			};											
		}
	],
	true,
	false
] call KH_fnc_execute;