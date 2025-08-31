params [["_side1", sideUnknown, [sideUnknown]], ["_side2", sideUnknown, [sideUnknown]], ["_allied", true, [true]]];
private _relationship = parseNumber _allied;
_side1 setFriend [_side2, _relationship];
_side2 setFriend [_side1, _relationship];