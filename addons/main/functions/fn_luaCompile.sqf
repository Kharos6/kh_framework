params [["_function", "", [""]], ["_name", "", [""]]];
if !(isNil {KH_var_cachedLuaFunctions get _name}) exitWith {};

if (_name isEqualTo "") then {
	_name = hashValue _function;
};

if (!(" " in _function) && (".lua" in _function)) then {
	_function = preprocessFile _function;
};

if !(isNil {["kh_framework", _this, "LuaCompile", false] call KH_fnc_callExtension;}) then {
	KH_var_cachedLuaFunctions set [_name, _function];
	nil;
};