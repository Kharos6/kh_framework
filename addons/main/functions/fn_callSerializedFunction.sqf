params ["_arguments", ["_function", "", ["", {}]], ["_caller", 2, [0]], ["_unscheduled", true, [true]]];

if (_function isEqualType {}) exitWith {
	if _unscheduled then {
		if (isNil "_arguments") then {
			call _function;
		}
		else {
			_arguments call _function;
		};
	}
	else {
		if (isNil "_arguments") then {
			[] spawn _function;
		}
		else {
			_arguments spawn _function;
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
			[] spawn _storedFunction;
		}
		else {
			_arguments spawn _storedFunction;
		};
	};
};

[
	[_arguments, _function, _caller, _unscheduled, true],
	"KH_fnc_processRemoteSerializedFUnction",
	"SERVER",
	true,
	[
		"CALLBACK",
		[_function],
		"KH_fnc_retrieveSerializedFunction"
	]
] call KH_fnc_execute;