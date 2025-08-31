params ["_arguments", ["_function", {}, ["", {}]], ["_expectedResult", true], ["_functionName", "TEMPORARY", [""]], ["_logOnlyMismatch", true, [true]], ["_simpleFormat", true, [true]]];
private _performance = 0;

private _result = if (_function isEqualType "") then {
	if (_functionName isEqualTo "TEMPORARY") then {
		_functionName = _function;
	};

	_performance = (diag_codePerformance [missionNamespace getVariable [_function, {}], _arguments]) select 0;
	_arguments call (missionNamespace getVariable [_function, {}]);
}
else {
	_performance = (diag_codePerformance [_function, _arguments]) select 0;
	_arguments call _function;
};

private _match = _result isEqualTo _expectedResult;

if (!_logOnlyMismatch || (_logOnlyMismatch && !_match)) then {
	if _simpleFormat then {
		diag_log (text ([
			"FUNCTION TEST: NAME = ",
			_functionName,
			" | MATCH = ",
			_match,
			" | PERFORMANCE = ",
			_performance,
			"MS | ARGUMENTS = ",
			_arguments,
			" | RESULT ",
			["(", typeName _result, ") = "] joinString "",
			_result, 
			" | EXPECTED RESULT ", 
			["(", typeName _expectedResult, ") = "] joinString "",
			_expectedResult
		] joinString ""));
	}
	else {
		diag_log (text "|FUNCTION TEST START|");
		diag_log (text (["      NAME = ", _functionName] joinString ""));
		diag_log (text (["      ARGUMENTS = ", _arguments] joinString ""));
		diag_log (text (["      RESULT ", ["(", typeName _result, ") = "] joinString "", _result] joinString ""));
		diag_log (text (["      EXPECTED RESULT ", ["(", typeName _expectedResult, ") = "] joinString "", _expectedResult] joinString ""));
		diag_log (text (["      MATCH = ", _match] joinString ""));
		diag_log (text (["      PERFORMANCE = ", _performance, "MS"] joinString ""));
		diag_log (text "|FUNCTION TEST END|");
	};
};

[_match, _peformance, _result, _expectedResult];