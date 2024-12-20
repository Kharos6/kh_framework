params [["_state", true]];

if (isNil "KH_var_missionSuspended") then {
	KH_var_missionSuspended = false;
};

KH_var_missionSuspended = _state;
publicVariable "KH_var_missionSuspended";

if _state then {
	private _curators = [];

	{
		_curators pushBack (getAssignedCuratorUnit _x);
	} forEach allCurators;

	{
		_x enableSimulationGlobal false;
	} forEach (KH_var_allEntities - _curators);

	setTimeMultiplier 0.1;

	[
		[_curators],
		{
			params ["_curators"];

			if !(player in _curators) then {
				titleText ["MISSION SUSPENDED - PLEASE WAIT", "BLACK IN", 999999];
			};
		},
		["JIP", "PLAYERS", "KH_var_missionSuspended", true, true],
		"THIS_FRAME"
	] call KH_fnc_execute;
}
else {
	private _curators = [];

	{
		_curators pushBack (getAssignedCuratorUnit _x);
	} forEach allCurators;

	{
		_x enableSimulationGlobal true;
	} forEach (KH_var_allEntities - _curators);

	setTimeMultiplier 1;

	[
		[_curators],
		{
			params ["_curators"];

			if !(player in _curators) then {
				titleText ["MISSION SUSPENDED - PLEASE WAIT", "BLACK IN", 1];
			};
		},
		"PLAYERS",
		"THIS_FRAME"
	] call KH_fnc_execute;
};