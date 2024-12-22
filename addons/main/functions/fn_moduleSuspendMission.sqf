isNil {
	params [["_logic", objNull], ["_units", []], ["_activated", false]];

	if _activated then {
		[
			[_logic getVariable ["KH_ModuleSuspendMissionSuspend", false]],
			{
				params ["_state"];
				[_state] call KH_fnc_suspendMission;
			},
			"SERVER",
			"THIS_FRAME"
		] call KH_fnc_execute;
	};
};

true;