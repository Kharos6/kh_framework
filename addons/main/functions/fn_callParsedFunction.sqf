params ["_arguments", ["_function", "", ["", {}]], ["_caller", 2, [0]], ["_unscheduled", true, [true]]];

if (_function isEqualType {}) exitWith {
	if _unscheduled then {
		if (isNil "_arguments") then {
			call _storedFunction;
		}
		else {
			_arguments call _storedFunction;
		};
	}
	else {
		if (isNil "_arguments") then {
			spawn _storedFunction;
		}
		else {
			_arguments spawn _storedFunction;
		};
	};
};

private _storedFunction = missionNamespace getVariable _function;

if !(isNil "_storedFunction") exitWith {
	if _unscheduled then {
		if (isNil "_arguments") then {
			call _storedFunction;
		}
		else {
			_arguments call _storedFunction;
		};
	}
	else {
		if (isNil "_arguments") then {
			spawn _storedFunction;
		}
		else {
			_arguments spawn _storedFunction;
		};
	};
};

[
	[_arguments, _function, _caller, _unscheduled],
	{
		params ["_arguments", "_function", "_caller", "_unscheduled"];
		_argsCallback params ["_storedFunction"];

		if !(isNil "_storedFunction") exitWith {
			missionNamespace setVariable [_function, _storedFunction];

			if _unscheduled then {
				if (isNil "_arguments") then {
					call _storedFunction;
				}
				else {
					_arguments call _storedFunction
				};
			}
			else {
				if (isNil "_arguments") then {
					spawn _storedFunction;
				}
				else {
					_arguments spawn _storedFunction;
				};
			};
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

					if _unscheduled then {
						if (isNil "_arguments") then {
							call _storedFunction;
						}
						else {
							_arguments call _storedFunction
						};
					}
					else {
						if (isNil "_arguments") then {
							spawn _storedFunction;
						}
						else {
							_arguments spawn _storedFunction;
						};
					};
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