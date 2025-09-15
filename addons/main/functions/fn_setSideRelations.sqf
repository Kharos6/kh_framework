params [["_side1", sideUnknown, [sideUnknown]], ["_side2", sideUnknown, [sideUnknown]], ["_relationship", true, [true, 0]]];

if (_relationship isEqualType true) then {
    _relationship = parseNumber _relationship;
};

_side1 setFriend [_side2, _relationship];
_side2 setFriend [_side1, _relationship];