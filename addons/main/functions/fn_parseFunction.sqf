params ["_function"];
private _parse = true;

if (_function isEqualType "") then {
	if (!(" " in _function) && !(".sqf" in _function)) then {
		_parse = false;
	};
};

if _parse then {
	private _hashValue = format ["KH_fnc_cachedFunction%1", hashValue _function];
	private _storedFunction = KH_var_cachedFunctions get _hashValue;

	if (isNil "_storedFunction") then {
		private _compiledFunction = _function;

		if (_function isEqualType "") then {
			if (".sqf" in _function) then {
				_compiledFunction = compile (preprocessFileLineNumbers _function);
			}
			else {
				_compiledFunction = compile _function;
			};
		};

		KH_var_cachedFunctions set [_hashValue, _compiledFunction, false];

		if !isServer then {
			[[_function], "KH_fnc_parseFunction", "SERVER", "THIS_FRAME"] call KH_fnc_execute;
		};
	};

	_function = _hashValue;
};

_function;