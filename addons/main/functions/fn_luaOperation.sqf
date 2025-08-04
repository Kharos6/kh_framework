params [["_arguments", [], [[]]], ["_function", "", [""]]];

if (_function isEqualTo "") exitWith {
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

if !(" " in _function) then {
	private _i = 1;
	private _argumentList = [];

	for "_i" from 1 to (count _arguments) do {
		_argumentList pushBack (["arg", _i] joinString "");
	};

	private _parsedArgumentList = [_argumentList] joinString "";
	_function = ["return ", _function, "(", (_parsedArgumentList select [1, ((_parsedArgumentList find "]") - 1)]) regexReplace ['"', ""], ")"] joinString "";
};

("kh_framework" callExtension ["LuaOperation", [_arguments, _function]]) params ["_result", "_returnCode"];

if ([_returnCode] call KH_fnc_parseBoolean) then {
	diag_log (text ([_result, " | EXTENSION = kh_framework | FUNCTION = LuaOperation | ARGUMENTS = ", [_arguments, _function]] joinString ""));
	nil;
}
else {
	_result;
};