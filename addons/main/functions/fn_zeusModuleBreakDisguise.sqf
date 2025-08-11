isNil {
	params [["_logic", objNull]];
	private _entity = attachedTo _logic;
	deleteVehicle _logic;

	if (isPlayer _entity) then {
		[
			[_entity],
			{
				params ["_player"];
				_player setVariable ["KH_var_disguiseState", false];
				_player setVariable ["KH_var_disguiseDetected", true];
			},
			"SERVER",
			true,
			false
		] call KH_fnc_execute;
	};
};

true;