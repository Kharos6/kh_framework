params [["_function", {}, ["", {}]], ["_public", false, [true]]];

private _parse = if (_function isEqualType "") then {
	((" " in _function) || (".sqf" in _function));
}
else {
	true;
};

if !_parse exitWith {
	_function;
};

private _hashValue = hashValue _function;

if (isNil {missionNamespace getVariable _hashValue;}) then {
	if (_function isEqualType "") then {
		if ((".sqf" in _function) && !(" " in _function)) then {
			_function = compileScript [_function, false, ""];
		}
		else {
			_function = compile _function;
		};
	};

	if _public then {
		missionNamespace setVariable [_hashValue, _function, true];
	}
	else {
		missionNamespace setVariable [_hashValue, _function];

		if !isServer then {
			missionNamespace setVariable [_hashValue, _function, 2];
		};
	};
}
else {
	if _public then {
		if ((".sqf" in _function) && !(" " in _function)) then {
			_function = compileScript [_function, false, ""];
		}
		else {
			_function = compile _function;
		};

		missionNamespace setVariable [_hashValue, _function, true];
	};
};

_hashValue;