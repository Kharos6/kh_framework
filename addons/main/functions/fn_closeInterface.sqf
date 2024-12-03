params [["_displays", []], ["_dialog", true]];

if _dialog then {
	closeDialog 0;
};

{
	if !(isNull (findDisplay _x)) then {
		(findDisplay _x) closeDisplay 2;
	};
} forEach _displays;

true;