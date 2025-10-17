params [["_state", true, [true]], ["_viewDistance", 0, [0]]];
KH_var_limitViewDistanceState = true;
publicVariable "KH_var_limitViewDistanceState";
KH_var_viewDistanceLimit = _viewDistance;
publicVariable "KH_var_viewDistanceLimit";

if _state then {
	if (isNil "KH_var_viewDistanceSet") then {
		KH_var_viewDistanceSet = true;
		publicVariable "KH_var_viewDistanceSet";
		
		[
			[], 
			{
				[
					[],
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
					true,
					0,
					false
				] call KH_fnc_execute;
			},
			"PLAYERS",
			true,
			["JIP", true, true, ""]
		] call KH_fnc_execute;
	};
};

nil;