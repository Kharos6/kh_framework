params ["_arguments", "_function"];
private _storedFunction = KH_var_cachedFunctions get _function;

if !(isNil "_storedFunction") exitWith {
	_storedFunction;
};

if ((missionNamespace getVariable [_function, {}]) isEqualTo {}) then {
	switch (count _arguments) do {
		case 0: {
			compile ([_function] joinString "");
		};

		case 1: {
			private _unaryArgument = call KH_fnc_generateUid;
			missionNamespace setVariable [_unaryArgument, _arguments select 0];
			compile ([_function, " (missionNamespace getVariable '", _unaryArgument, "');"] joinString "");
		};

		case 2: {
			private _binaryArguments = call KH_fnc_generateUid;
			missionNamespace setVariable [_binaryArguments, _arguments];
			compile (["((missionNamespace getVariable '", _binaryArguments, "') select 0) ", _function, " ((missionNamespace getVariable '", _binaryArguments, "') select 1);"] joinString "");
		};

		default {
			{};
		};
	};
};