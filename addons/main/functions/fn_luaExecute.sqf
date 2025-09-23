params ["_arguments", ["_function", "", [""]], ["_rawReturn", true, [true]]];
private _parsedArguments = [];

if (_arguments isEqualType []) then {
	{
		_parsedArguments pushBack (["", _x] call KH_fnc_serializeValue);
	} forEach _arguments;
}
else {
	_parsedArguments pushBack (["", _arguments] call KH_fnc_serializeValue);
};

if _rawReturn exitWith {
	["kh_framework", ["ExecuteLua", [_function, _parsedArguments]], "CommunicateLuaFramework", false] call KH_fnc_callExtension;
};

private _return = [parseSimpleArray (["kh_framework", ["ExecuteLua", [_function, _parsedArguments]], "CommunicateLuaFramework", false] call KH_fnc_callExtension), []] call KH_fnc_parseNestedTypedArray;

if ((count _return) isEqualTo 1) then {
	_return select 0;
}
else {
	_return;
};