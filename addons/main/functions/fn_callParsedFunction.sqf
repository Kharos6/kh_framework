params ["_arguments", ["_function", "", [""]]];
private _storedFunction = KH_var_cachedFunctions get _function;

if !(isNil "_storedFunction") exitWith {
	_arguments call _storedFunction;
};

if ("KH_fnc_cachedFunction_" in _function) then {
	private _caller = param [2, clientOwner, [0]];

	[
		[_arguments, _function, _caller],
		{
			params ["_arguments", "_function", "_caller"];
			_argsCallback params ["_storedFunction"];

			if (_storedFunction isNotEqualTo {}) then {
				KH_var_cachedFunctions set [_function, _storedFunction, false];
				_arguments call _storedFunction;
			}
			else {
				[
					[_arguments, _function],
					{
						params ["_arguments", "_function"];
						_argsCallback params ["_storedFunction"];

						if (_storedFunction isNotEqualTo {}) then { 
							KH_var_cachedFunctions set [_function, _storedFunction, false];
							_arguments call _storedFunction;
						};
					},
					[
						"CALLBACK", 
						_caller, 
						[_function],
						{
							params ["_function"];
							private _storedFunction = KH_var_cachedFunctions get _function;
							
							if !(isNil "_storedFunction") then {
								[_storedFunction];
							}
							else {
								[{}];
							};											
						}
					],
					"THIS_FRAME"
				] call KH_fnc_execute;
			};
		},
		[
			"CALLBACK", 
			"SERVER", 
			[_function],
			{
				params ["_function"];
				private _storedFunction = KH_var_cachedFunctions get _function;
				
				if !(isNil "_storedFunction") then {
					[_storedFunction];
				}
				else {
					[{}];
				};											
			}
		],
		"THIS_FRAME"
	] call KH_fnc_execute;

	nil;
}
else {
	private _parsedFunction = missionNamespace getVariable _function;

	if !(isNil "_parsedFunction") exitWith {
		_arguments call _parsedFunction;
	};

	switch (count _arguments) do {
		case 0: {
			call (compile ([_function] joinString ""));
		};

		case 1: {
			private _unaryArgument = call KH_fnc_generateUid;
			missionNamespace setVariable [_unaryArgument, _arguments select 0];
			call (compile ([_function, " (missionNamespace getVariable '", _unaryArgument, "');"] joinString ""));
		};

		case 2: {
			private _binaryArguments = call KH_fnc_generateUid;
			missionNamespace setVariable [_binaryArguments, _arguments];
			call (compile (["((missionNamespace getVariable '", _binaryArguments, "') select 0) ", _function, " ((missionNamespace getVariable '", _binaryArguments, "') select 1);"] joinString ""));
		};

		default {
			nil;
		};
	};
};