isNil {
	params [["_logic", objNull]];
	deleteVehicle _logic;

	if !(isNil "KH_var_stagedExecutions") then {
		{
			[_x] call KH_fnc_removeEventHandler;
		} forEach KH_var_stagedExecutions;
	};
};

true;