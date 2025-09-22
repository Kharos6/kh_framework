params [["_function", "", [""]]];

if (!(" " in _function) && (".lua" in _function)) then {
	_function = preprocessFile _function;
};

KH_var_queuedLuaExecutions pushBack _function;