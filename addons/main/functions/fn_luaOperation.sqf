params [["_arguments", [], [[]]], ["_function", "", [""]]];

if (_function isEqualTo "") exitWith {
	nil;
};

private _parsedFunction = if (!(" " in _function) && !(".lua" in _function)) then {
	KH_var_cachedLuaFunctions get _function;
}
else {
	KH_var_cachedLuaFunctions get (hashValue _function);
};

private _continue = if (isNil "_parsedFunction") then {
	_parsedFunction = [_function, ""] call KH_fnc_luaCompile;

	if (isNil "_parsedFunction") then {
		false;
	}
	else {
		true;
	};
}
else {
	true;
};

if !_continue exitWith {
	nil;
};

private _parsedArguments = [];

{
	switch (typeName _x) do {
		case "OBJECT";
		case "GROUP";
		case "SIDE": {
			private _entityHash = ["KH_var_cachedLuaEntity_", hashValue _x] joinString "";
			KH_var_cachedLuaEntities set [_entityHash, _x];
			_parsedArguments pushBack _entityHash;
		};

		default {
			_parsedArguments pushBack _x;
		};
	};
} forEach _arguments;

("kh_framework" callExtension ["LuaOperation", [_arguments, _parsedFunction]]) params ["_result", "_returnCode"];

if ([_returnCode] call KH_fnc_parseBoolean) then {
	diag_log (text ([_result, " | EXTENSION = kh_framework | FUNCTION = LuaOperation | ARGUMENTS = ", [_arguments, _parsedFunction]] joinString ""));
	nil;
}
else {
	_result;
};