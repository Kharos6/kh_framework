params [["_feature", [], ["", []]], ["_extension", "", [""]], ["_expectedResult", ""], ["_logOnlyMismatch", true, [true]], ["_simpleFormat", true, [true]]];
private "_arguments";

private _function = if (_feature isEqualType []) then {
	_arguments = flatten (_feature select [1]);
	_feature select 0;
}
else {
	_arguments = [];
	_feature;
};

private _performance = (diag_codePerformance [{_extension callExtension _feature}]) select 0;
(_extension callExtension _feature) params ["_result", "_returnCode", "_errorCode"];

private _parsedExpectedResult = if (_expectedResult isEqualType "") then {
	_expectedResult;
}
else {
	[_expectedResult] joinString "";
};

private _match = if (_result isEqualTo _parsedExpectedResult) then {
	"true";
}
else {
	"false";
};

if (!_logOnlyMismatch || (_logOnlyMismatch && !_match)) then {
	if _simpleFormat then {
		diag_log (text ([
			"EXTENSION TEST: NAME = ",
			_extension,
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
		diag_log (text (["      NAME = ", _extension] joinString ""));
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