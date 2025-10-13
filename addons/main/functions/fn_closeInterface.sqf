params [["_display", displayNull, [0, displayNull]], ["_dialog", true, [true]]];

if _dialog then {
	closeDialog 0;
};

if (_display isEqualType 0) then {
	if !(isNull (findDisplay _display)) then {
		(findDisplay _display) closeDisplay 2;
	};
}
else {
	if !(isNull _display) then {
		_display closeDisplay 2;
	};
};