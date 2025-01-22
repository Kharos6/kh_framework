params ["_arguments", "_function", ["_caller", clientOwner]];
private _storedFunction = KH_var_cachedFunctions get _function;

if (isNil "_storedFunction") then {
	if ("KH_fnc_cachedFunction" in _function) then {
		[
			[_arguments, _function, _caller],
			{
				params ["_arguments", "_function", "_caller"];
				_argsCallback params ["_storedFunction"];

				if (_storedFunction isNotEqualTo {}) then { 
					_arguments call _storedFunction;
					KH_var_cachedFunctions set [_function, _storedFunction, false];
				}
				else {
					[
						[_arguments, _function],
						{
							params ["_arguments", "_function"];
							_argsCallback params ["_storedFunction"];

							if (_storedFunction isNotEqualTo {}) then { 
								_arguments call _storedFunction;
								KH_var_cachedFunctions set [_function, _storedFunction, false];
							};
						},
						[
							"CALLBACK", 
							_caller, 
							[_function],
							{
								params ["_function"];
								private _storedFunction = KH_var_cachedFunctions get _function;
								
								if !(isNil _storedFunction) then {
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
					
					if !(isNil _storedFunction) then {
						[_storedFunction];
					}
					else {
						[{}];
					};											
				}
			],
			"THIS_FRAME"
		] call KH_fnc_execute;

		true;
	}
	else {
		private _parsedFunction = missionNamespace getVariable [_function, {}];

		if (_parsedFunction isEqualTo {}) then {
			switch true do {
				case ((count _arguments) == 0): {
					[] call (compile ([_function] joinString ""));
				};

				case ((count _arguments) == 1): {
					private _unaryArgument = [] call KH_fnc_generateUid;
					missionNamespace setVariable [_unaryArgument, _arguments select 0];
					[] call (compile ([_function, " (missionNamespace getVariable '", _unaryArgument, "');"] joinString ""));
				};

				case ((count _arguments) == 2): {
					private _binaryArguments = [] call KH_fnc_generateUid;
					missionNamespace setVariable [_binaryArguments, _arguments];
					[] call (compile (["((missionNamespace getVariable '", _binaryArguments, "') select 0) ", _function, " ((missionNamespace getVariable '", _binaryArguments, "') select 1);"] joinString ""));
				};
			};
		}
		else {
			_arguments call _parsedFunction;
		};
	};
}
else {
	_arguments call _storedFunction;
};