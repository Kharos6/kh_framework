params ["_side1", "_side2", "_allied"];
private _relationship = 0;

if _allied then {
	_relationship = 1;
};

_side1 setFriend [_side2, _relationship];
_side2 setFriend [_side1, _relationship];
true;