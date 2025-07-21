params [["_count", 16, [0]], ["_numbers", true, [true]], ["_letters", true, [true]], ["_symbols", false, [true]]];

if ((_count <= 0) || (!_numbers && !_letters && !_symbols)) exitWith {
	nil;
};

if (_count <= 8192) exitWith {
	("kh_framework" callExtension ["GenerateRandomString", _this]) params ["_result", "_returnCode"];

	if ([_returnCode] call KH_fnc_parseBoolean) then {
		diag_log (text ([_result, " | EXTENSION = kh_framework | FUNCTION = GenerateRandomString | ARGUMENTS = ", _this] joinString ""));
		nil;
	}
	else {
		_result;
	};
};

private _strings = [];
private _i = 1;
private _break = false;

for "_i" from 1 to (ceil (_count / 8192)) do {
	private _remainder = _count - (_i - 1) * 8192;

	if (_remainder > 8192) then { 
		_remainder = 8192; 
	};

	("kh_framework" callExtension ["GenerateRandomString", [_remainder, _this select 1, _this select 2, _this select 3]]) params ["_currentResult", "_returnCode"];

	if ([_returnCode] call KH_fnc_parseBoolean) then {
		diag_log (text ([_currentResult, " | EXTENSION = kh_framework | FUNCTION = GenerateRandomString | ARGUMENTS = ", [_remainder, _this select 1, _this select 2, _this select 3]] joinString ""));
		_break = true;
		break;
	};
	
	_strings pushBack _currentResult;
};

if _break exitWith {
	nil;
};

_strings joinString "";