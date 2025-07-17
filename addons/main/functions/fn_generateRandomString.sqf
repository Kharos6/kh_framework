params [["_count", 16, [0]], ["_numbers", true, [true]], ["_letters", true, [true]], ["_symbols", false, [true]]];

if (!_numbers && !_letters && !_symbols) exitWith {
	false;
};

if (_count <= 8192) exitWith {
	("kh_framework" callExtension ["GenerateRandomString", _this]) select 0;
};

private _strings = [];
private _i = 1;

for "_i" from 1 to (ceil (_count / 8192)) do {
	private _remainder = _count - (_i - 1) * 8192;

	if (_remainder > 8192) then { 
		_remainder = 8192; 
	};
	
	_strings pushBack (("kh_framework" callExtension ["GenerateRandomString", [_remainder, _this select 1, _this select 2, _this select 3]]) select 0);
};

private _result = _strings joinString "";

if ("KH_ERROR: " in _result) then {
	false;
}
else {
	_result;
};