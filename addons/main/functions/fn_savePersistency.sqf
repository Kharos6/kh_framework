params [["_identifier", "", [""]], ["_unit", true, [true]], ["_group", true, [true]], ["_object", true, [true]], ["_player", true, [true]], ["_mission", true, [true]]];

if _unit then {
    [_identifier, allUnits] call KH_fnc_saveUnitPersistency;
};

if _group then {
    [_identifier, allGroups] call KH_fnc_saveGroupPersistency;
};

if _object then {
    [_identifier, entities [[], ["Man"], false, true]] call KH_fnc_saveObjectPersistency;
};

if _player then {
    [_identifier, KH_var_allPlayerUnits] call KH_fnc_savePlayerPersistency;
};

if _mission then {
    call KH_fnc_saveMissionPersistency;
};