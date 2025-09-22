params [["_arguments", [], [[]]], ["_function", "", [""]]];
private _parsedArguments = [];

{
	_parsedArguments pushBack (["", _x] call KH_fnc_serializeValue);
} forEach _arguments;

private _return = parseSimpleArray (["kh_framework", [_parsedArguments, _function], "LuaOperation", false] call KH_fnc_callExtension);

if ((count _return) isEqualTo 1) then {
	_return select 0;
}
else {
	_return;
};