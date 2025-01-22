params ["_arguments", "_function"];
private _storedFunction = KH_var_cachedFunctions get _function;

if (isNil "_storedFunction") then {
	private _parsedFunction = missionNamespace getVariable [_function, {}];

	if (_parsedFunction isEqualTo {}) then {
		switch true do {
			case ((count _arguments) == 0): {
				_parsedFunction = compile ([_function] joinString "");
			};

			case ((count _arguments) == 1): {
				private _unaryArgument = [] call KH_fnc_generateUid;
				missionNamespace setVariable [_unaryArgument, _arguments select 0];
				_parsedFunction = compile ([_function, " (missionNamespace getVariable '", _unaryArgument, "');"] joinString "");
			};

			case ((count _arguments) == 2): {
				private _binaryArguments = [] call KH_fnc_generateUid;
				missionNamespace setVariable [_binaryArguments, _arguments];
				_parsedFunction = compile (["((missionNamespace getVariable '", _binaryArguments, "') select 0) ", _function, " ((missionNamespace getVariable '", _binaryArguments, "') select 1);"] joinString "");
			};
		};
	};

	_parsedFunction;
}
else {
	_storedFunction;
};