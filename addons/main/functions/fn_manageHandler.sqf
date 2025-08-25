params [["_id", [], [[]]], ["_state", true, [true, ""]]];
_id params [["_namespace", objNull, [missionNamespace, objNull, grpNull, teamMemberNull, locationNull, taskNull, controlNull, displayNull]], ["_variable", "", [""]], ["_target", false, [true, 0, []]]];

if (_state isEqualType true) then {
	_state = if _state then {
		"ACTIVE";
	}
	else {
		"INACTIVE";
	};
};

_namespace setVariable [_variable, _state, _target];