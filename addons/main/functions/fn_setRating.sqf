params [["_unit", objNull, [objNull]], ["_rating", true, [true, 0]]];

if (_rating isEqualType true) then {
    _rating = [-999999, 999999] select _rating;
};

_unit addRating (_rating - (rating _unit));