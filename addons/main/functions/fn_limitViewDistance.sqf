params ["_state", "_viewDistance"];
KH_var_viewDistanceLimit = _viewDistance;
publicVariable "KH_var_viewDistanceLimit";

if _state then {
	KH_var_limitViewDistanceState = true;
	publicVariable "KH_var_limitViewDistanceState";

	if (isNil "KH_var_viewDistanceSet") then {
		KH_var_viewDistanceSet = true;
		
		[
			[], 
			{
				[
					{
						if KH_var_limitViewDistanceState then {
							if (viewDistance > KH_var_viewDistanceLimit) then {
								setViewDistance KH_var_viewDistanceLimit;
							};
							
							if ((getObjectViewDistance select 0) > KH_var_viewDistanceLimit) then {
								setObjectViewDistance KH_var_viewDistanceLimit;
							};
						};
					},
					0, 
					[]
				] call CBA_fnc_addPerFrameHandler;
			},
			["JIP", "PLAYERS", objNull, false, false],
			"THIS_FRAME"
		] call KH_fnc_execute;
	};
}
else {
	KH_var_limitViewDistanceState = false;
	publicVariable "KH_var_limitViewDistanceState";
};

true;