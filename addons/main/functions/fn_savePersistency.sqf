params [["_identifier", "", [""]], ["_unit", [true, false], [[]]], ["_group", true, [true]], ["_object", [true, false], [[]]], ["_player", [true, true], [[]]], ["_marker", true, [true]], ["_mission", true, [true]]];
_unit params [["_saveUnit", true, [true]], ["_saveUnitUndefined", false, [true]]];
_object params [["_saveObject", true, [true]], ["_saveObjectUndefined", false, [true]]];
_player params [["_savePlayer", true, [true]], ["_saveDisconnected", true, [true]]];

if _saveObject then {
    [_identifier, (allMissionObjects "") select {!(_x isKindOf "Man") && !(_x isKindOf "Logic") && !(_x isKindOf "EmptyDetector") && (((typeOf _x) select [0, 1]) isNotEqualTo "#");}, _saveObjectUndefined] call KH_fnc_saveObjectPersistency;
};

if _saveUnit then {
    [_identifier, KH_var_allMen, _saveUnitUndefined] call KH_fnc_saveUnitPersistency;
};

if _savePlayer then {
    [_identifier, KH_var_allPlayerUnits, _saveDisconnected] call KH_fnc_savePlayerPersistency;
};

if _group then {
    [_identifier, allGroups] call KH_fnc_saveGroupPersistency;
};

if _marker then {
    [_identifier, keys KH_var_allMarkers] call KH_fnc_saveMarkerPersistency;
};

if _mission then {
    [_identifier] call KH_fnc_saveMissionPersistency;
};