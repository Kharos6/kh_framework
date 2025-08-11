params [["_function", {}, ["", {}]]];

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

private _hashValue = hashValue _function;

if (isNil {missionNamespace getVariable _hashValue;}) then {
	missionNamespace setVariable [
		_hashValue, 
		if (_function isEqualType "") then {
			if ((".sqf" in _function) && !(" " in _function)) then {
				compileScript [_function, false, ""];
			}
			else {
				compile _function;
			};
		}
		else {
			_function;
		}, 
		if !isServer then {
			[clientOwner, 2];
		}
		else {
			false;
		}
	];
};

_hashValue;