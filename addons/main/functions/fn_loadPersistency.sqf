params [["_identifier", "", [""]], ["_unit", true, [true, []]], ["_group", true, [true, []]], ["_object", true, [true, []]], ["_player", true, [true, []]], ["_mission", true, [true]]];

if (_object isEqualType true) then {
    if _object then {
        [_identifier, []] call KH_fnc_loadObjectPersistency;
    };
}
else {
    _object call KH_fnc_loadObjectPersistency;
};

if (_unit isEqualType true) then {
    if _unit then {
        [_identifier, []] call KH_fnc_loadUnitPersistency;
    };
}
else {
    _unit call KH_fnc_loadUnitPersistency;
};

if (_player isEqualType true) then {
    if _player then {
        [_identifier, "INITIAL", []] call KH_fnc_loadPlayerPersistency;
    };
}
else {
    _player call KH_fnc_loadPlayerPersistency;
};

if (_group isEqualType true) then {
    if _group then {
        [_identifier, []] call KH_fnc_loadGroupPersistency;
    };
}
else {
    _group call KH_fnc_loadGroupPersistency;
};

if _mission then {
    [_identifier] call KH_fnc_loadMissionPersistency;
};