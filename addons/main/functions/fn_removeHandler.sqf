params [["_id", [], [[]]]];

_id params [
    ["_namespace", objNull, [missionNamespace, objNull, grpNull, teamMemberNull, locationNull, taskNull, controlNull, displayNull]], 
    ["_variable", "", [""]], 
    ["_target", false, [true, 0, []]],
    ["_event", [], [[]]]
];

_namespace setVariable [_variable, false, _target];

if (_event isNotEqualTo []) then {
    [_event] call KH_fnc_removeEventHandler;
};