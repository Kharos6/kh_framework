params [["_repeatCount", 0], ["_length", 36], ["_idType", "ALPHANUMERIC"]];
private _numbers = ["0", "1", "2", "3", "4", "5", "6", "7", "8", "9"];
private _letters = ["a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z"];
private _symbols = [];
private _i = 0;

switch true do {
	case (_idType == "ALPHABETIC"): {
		if (_repeatCount > 0) then {
			for "_i" from 1 to _repeatCount do {
				private _lettersNew = _letters + ["a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z"];
				_letters = _lettersNew;
			};
		};
		
		_symbols = _letters;
	};
	
	case (_idType == "NUMERIC"): {
		if (_repeatCount > 0) then {
			for "_i" from 1 to _repeatCount do {
				private _numbersNew = _numbers + ["0", "1", "2", "3", "4", "5", "6", "7", "8", "9"];
				_numbers = _numbersNew;
			};
		};
		
		_symbols = _numbers;
	};
	
	case (_idType == "ALPHANUMERIC"): {
		_symbols = _numbers + _letters;
		
		if (_repeatCount > 0) then {
			for "_i" from 1 to _repeatCount do {
				private _symbolsNew = _symbols + ["0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z"];
				_symbols = _symbolsNew;
			};
		};
	};
};

((_symbols call BIS_fnc_arrayShuffle) select [0, _length]) joinString "";