params [["_count", 36], ["_numbers", true], ["_letters", true], ["_symbols", false]];

if (_count > 8192) then {
	private _strings = [];
	private _i = 1;

	for "_i" from 1 to (ceil (_count / 8192)) do {
		private _remainder = _count - (_i - 1) * 8192;

		if (_remainder > 8192) then { 
			_remainder = 8192; 
		};
		
		_strings pushBack (("kh_framework" callExtension ["GenerateRandomString", [_remainder, _this select 1, _this select 2, _this select 3]]) select 0);
	};

	_strings joinString "";
}
else {
	("kh_framework" callExtension ["GenerateRandomString", _this]) select 0;
};