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
	_result params ["_value", "_executions"];

	{
		private _args = parseSimpleArray (_x select 0);
		private _parsedArgs = [];

		{
			if (_x isEqualType "") then {
				if ("KH_var_cachedLuaEntity_" in _x) then {
					_parsedArgs pushBack (KH_var_cachedLuaEntities get _x);
				}
				else {
					_parsedArgs pushBack _x;
				};
			}
			else {
				_parsedArgs pushBack _x;
			};
		} forEach _args;

		[_parsedArgs, [_x select 1] call KH_fnc_parseFunction, clientOwner] call KH_fnc_callParsedFunction;
	} forEach (parseSimpleArray _executions);

	_value;
};