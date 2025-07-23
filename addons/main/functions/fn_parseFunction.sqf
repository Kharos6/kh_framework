params ["_function"];

private _parse = if (_function isEqualType "") then {
	if (!(" " in _function) && !(".sqf" in _function)) then {
		false;
	}
	else {
		true;
	};
}
else {
	true;
};

if !_parse exitWith {
	_function;
};

private _hashValue = format ["KH_fnc_cachedFunction_%1", hashValue _function];

if (isNil {KH_var_cachedFunctions get _hashValue}) then {
	KH_var_cachedFunctions set [
		_hashValue, 
		if (_function isEqualType "") then {
			if (".sqf" in _function) then {
				compile (preprocessFileLineNumbers _function);
			}
			else {
				compile _function;
			};
		}
		else {
			_function;
		}, 
		false
	];

	if !isServer then {
		[[_function], "KH_fnc_parseFunction", "SERVER", "THIS_FRAME"] call KH_fnc_execute;
	};
};

_hashValue;