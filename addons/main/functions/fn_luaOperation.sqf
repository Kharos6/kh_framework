params [["_arguments", [], [[]]], ["_function", "", [""]]];
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

		case "TEXT";
		case "STRING": {
			_parsedArguments pushBack _x;
		};

		default {
			_parsedArguments pushBack (str _x);
		};
	};
} forEach _arguments;

if !(" " in _function) then {
	private _i = 1;
	private _argumentList = [];

	for "_i" from 1 to (count _parsedArguments) do {
		_argumentList pushBack (["arg", _i] joinString "");
	};

	private _parsedArgumentList = [_argumentList] joinString "";
	_function = ["return ", _function, "(", (_parsedArgumentList select [1, ((_parsedArgumentList find "]") - 1)]) regexReplace ['"', ""], ")"] joinString "";
};

("kh_framework" callExtension ["LuaOperation", [_parsedArguments, _function]]) params ["_result", "_returnCode"];

if ([_returnCode] call KH_fnc_parseBoolean) then {
	diag_log (text ([_result, " | EXTENSION = kh_framework | FUNCTION = LuaOperation | ARGUMENTS = ", [_parsedArguments, _function]] joinString ""));
}
else {
	_result;
};