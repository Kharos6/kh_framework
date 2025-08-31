params ["_arguments", ["_function", "", [""]], ["_expectedResult", true], ["_functionName", "TEMPORARY", [""]], ["_stateType", "WARM", [""]], ["_logOnlyMismatch", true, [true]], ["_simpleFormat", true, [true]]];
private _performance = 0;

switch _stateType do {
	case "COLD": {
		_performance = (diag_codePerformance [
			{
				params ["_arguments", "_function"];
				call KH_fnc_luaResetState;
				"kh_framework" callExtension ["LuaOperation", [_arguments, _function]];
			},
			[_arguments, _function]
		]) select 0;
	};

	case "WARM": {
		[_function, _functionName] call KH_fnc_luaCompile;

		if !(" " in _function) then {
			private _i = 1;
			private _argumentList = [];

			for "_i" from 1 to (count _arguments) do {
				_argumentList pushBack (["arg", _i] joinString "");
			};

			private _parsedArgumentList = [_argumentList] joinString "";
			_function = ["return ", _function, "(", (_parsedArgumentList select [1, ((_parsedArgumentList find "]") - 1)]) regexReplace ['"', ""], ")"] joinString "";
		};

		_performance = (diag_codePerformance [
			{
				params ["_arguments", "_function"];
				"kh_framework" callExtension ["LuaOperation", [_arguments, _function]];
			},
			[_arguments, _function]
		]) select 0;
	};

	case "CURRENT": {
		_performance = (diag_codePerformance [
			{
				params ["_arguments", "_function"];
				"kh_framework" callExtension ["LuaOperation", [_arguments, _function]];
			},
			[_arguments, _function],
			1
		]) select 0;
	};
};

("kh_framework" callExtension ["LuaOperation", [_arguments, _function]]) params ["_result", "_returnCode", "_errorCode"];

private _parsedExpectedResult = if (_expectedResult isEqualType "") then {
	_expectedResult;
}
else {
	[_expectedResult] joinString "";
};

private _match = _result isEqualTo _expectedResult;

if (!_logOnlyMismatch || (_logOnlyMismatch && !_match)) then {
	if _simpleFormat then {
		diag_log (text ([
			"EXTENSION TEST: NAME = ",
			_functionName,
			" | MATCH = ",
			_match,
			" | RETURN CODE = ",
			_returnCode,
			" | ERROR CODE = ",
			_errorCode,
			" | FUNCTION = ",
			_function,
			" | PERFORMANCE = ",
			_performance,
			"MS | ARGUMENTS = ",
			_arguments,
			" | RESULT (STRING) = ",
			_result, 
			" | EXPECTED RESULT (STRING) = ", 
			_parsedExpectedResult
		] joinString ""));
	}
	else {
		diag_log (text "|EXTENSION TEST START|");
		diag_log (text (["      NAME = ", _functionName] joinString ""));
		diag_log (text (["      ARGUMENTS = ", _arguments] joinString ""));
		diag_log (text (["      FUNCTION = ", _function] joinString ""));
		diag_log (text (["      RESULT (STRING) = ", _result] joinString ""));
		diag_log (text (["      EXPECTED RESULT (STRING) = ", _parsedExpectedResult] joinString ""));
		diag_log (text (["      RETURN CODE = ", _returnCode] joinString ""));
		diag_log (text (["      ERROR CODE = ", _errorCode] joinString ""));
		diag_log (text (["      MATCH = ", _match] joinString ""));
		diag_log (text (["      PERFORMANCE = ", _performance, "MS"] joinString ""));
		diag_log (text "|EXTENSION TEST END|");
	};
};

[_match, _performance, _result, _parsedExpectedResult, _returnCode, _errorCode];