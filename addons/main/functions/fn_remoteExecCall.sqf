params [["_arguments", []], ["_function", "", [""]], ["_target", 0, [0, objNull, grpNull, "", sideUnknown, []]], ["_jip", false, [true, "", objNull, []]], ["_useKhExecuteOnFailure", true, [true]]];

if (
	(isNil {KH_var_remoteExecFunctionsBlacklist get _function}) && 
	(isNil {KH_var_remoteExecCommandsBlacklist get _function}) &&
	(
	 ((KH_var_remoteExecCommandsMode != 0) && (isNil {missionNamespace getVariable _function})) ||
	 ((KH_var_remoteExecFunctionsMode != 0) && !(isNil {missionNamespace getVariable _function}))
	) && 
	(
	 (
	  ((KH_var_remoteExecCommandsJipMode == 1) && (isNil {missionNamespace getVariable _function})) ||
	  ((KH_var_remoteExecFunctionsJipMode == 1) && !(isNil {missionNamespace getVariable _function}))
	 ) ||
	 (_jip isEqualTo false)
	)
   ) then {
	_arguments remoteExecCall [_function, _target, _jip];
}
else {
	if _useKhExecuteOnFailure then {
		[
			_arguments, 
			_function, 
			if (_jip isEqualTo false) then {
				_target;
			}
			else {
				["JIP", "GLOBAL", _jip, false, false, ["sha256", [_jip] joinString ""] call KH_fnc_cryptoOperation];
			},
			true
		] call KH_fnc_execute;
	};

	diag_log (text ([
		"WARNING: remoteExecCall failed due to CfgRemoteExec blacklist: ", 
		_function,
		if _useKhExecuteOnFailure then {
			" | Used KH_fnc_execute instead.";
		}
		else {
			"";
		}
	] joinString ""));
};