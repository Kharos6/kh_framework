params [
	"_arguments", 
	["_function", "", [""]], 
	["_target", 0, [0, objNull, teamMemberNull, grpNull, "", sideUnknown, []]], 
	["_jip", false, [true, "", objNull, grpNull]], 
	["_allowBlacklisted", true, [true]],
	["_unscheduled", true, [true]]
];

if (_target isEqualType teamMemberNull) then {
	_target = agent _target;
};

if isServer exitWith {
	if _unscheduled then {
		_arguments remoteExecCall [_function, _target, _jip];
	}
	else {
		_arguments remoteExec [_function, _target, _jip];
	};
};

if (
	(isNil {KH_var_remoteExecFunctionsBlacklist get _function}) && 
	(isNil {KH_var_remoteExecCommandsBlacklist get _function}) &&
	(
	 ((KH_var_remoteExecCommandsMode isNotEqualTo 0) && (isNil {missionNamespace getVariable _function})) ||
	 ((KH_var_remoteExecFunctionsMode isNotEqualTo 0) && !(isNil {missionNamespace getVariable _function}))
	) && 
	(
	 (
	  ((KH_var_remoteExecCommandsJipMode isEqualTo 1) && (isNil {missionNamespace getVariable _function})) ||
	  ((KH_var_remoteExecFunctionsJipMode isEqualTo 1) && !(isNil {missionNamespace getVariable _function}))
	 ) ||
	 (_jip isEqualTo false)
	)
   ) then {
	if _unscheduled then {
		_arguments remoteExecCall [_function, _target, _jip];
	}
	else {
		_arguments remoteExec [_function, _target, _jip];
	};
}
else {
	if _allowBlacklisted then {
		["KH_eve_execution", [_this, "KH_fnc_remoteExec", clientOwner]] call CBA_fnc_serverEvent;
	};
};