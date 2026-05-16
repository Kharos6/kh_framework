params [["_first", sideUnknown, [objNull, grpNull, sideUnknown]], ["_second", sideUnknown, [objNull, grpNull, sideUnknown]], ["_returnMode", true, [true]]];

if !(_first isEqualType sideUnknown) then {
    if (_first isEqualType objNull) then {
        _first = side (group _first);
    }
    else {
        _first = side _first;
    };
};

if !(_second isEqualType sideUnknown) then {
    if (_second isEqualType objNull) then {
        _second = side (group _second);
    }
    else {
        _second = side _second;
    };
};

if _returnMode then {
    (_first getFriend _second) >= 0.6;
}
else {
    if (_first isEqualTo _second) then {
        ["SAME_HOSTILE", "SAME_FRIENDLY"] select ((_first getFriend _second) >= 0.6);
    }
    else {
        ["HOSTILE", "FRIENDLY"] select ((_first getFriend _second) >= 0.6);
    };
};