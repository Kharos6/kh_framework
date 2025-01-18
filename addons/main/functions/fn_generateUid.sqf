private _symbols = [0, 36, "ALPHANUMERIC"] call KH_fnc_generateSymbols;
private _firstSymbol = selectRandom ["a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z"];
format ["%1_%2_%3_%4_%5", _firstSymbol, _symbols, diag_frameNo toFixed 0, (systemTime select [3]) joinString "", clientOwner];