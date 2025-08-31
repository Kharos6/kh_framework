params [["_count", 1, [0]], ["_numbers", true, [true]], ["_letters", true, [true]], ["_symbols", false, [true]]];

if (_count <= 8192) exitWith {
	["kh_framework", _this, "GenerateRandomString", false] call KH_fnc_callExtension;
};

private _strings = [];
private _i = 1;

for "_i" from 1 to (ceil (_count / 8192)) do {
	private _remainder = _count - (_i - 1) * 8192;

	if (_remainder > 8192) then { 
		_remainder = 8192; 
	};

	_strings pushBack (["kh_framework", [_remainder, _this select 1, _this select 2, _this select 3], "GenerateRandomString", false] call KH_fnc_callExtension);
};

_strings joinString "";