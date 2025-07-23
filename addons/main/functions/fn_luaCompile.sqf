params [["_function", "", [""]], ["_name", "", [""]]];

if (_function isEqualTo "") exitWith {
	nil;
};

private _hashValue = if (_name isEqualTo "") then { 
	hashValue _function;
}
else {
	_name;
};

if !(isNil {KH_var_cachedLuaFunctions get _hashValue}) exitWith {
	true;
};

if (".lua" in _function) then {
	_function = preprocessFile _function;
};

("kh_framework" callExtension ["LuaCompile", [_function]]) params ["_result", "_returnCode"];

if ([_returnCode] call KH_fnc_parseBoolean) then {
	diag_log (text ([_result, " | EXTENSION = kh_framework | FUNCTION = LuaCompile | ARGUMENTS = ", [_function]] joinString ""));
	nil;
}
else {
	missionNamespace setVariable [_hashValue, _function];
	KH_var_cachedLuaFunctions set [_hashValue, _function];
	true;
};