isNil {
	params [["_display", displayNull], ["_logic", objNull]];

	[
		["CONTROL", _display displayCtrl 1],
		"ButtonClick",
		[_display, _logic],
		{
			params ["_control"];
			_args params ["_display", "_logic"];

			[
				[cbChecked (_display displayCtrl 100), _logic],
				{
					params ["_state", "_logic"];
					[_state] call KH_fnc_suspendMission;
					deleteVehicle _logic;
				},
				"SERVER",
				true,
				false
			] call KH_fnc_execute;

			[_localId] call KH_fnc_removeEventHandler;
			nil;
		}
	] call KH_fnc_addEventHandler;

	[
		["CONTROL", _display displayCtrl 2],
		"ButtonClick",
		[_logic],
		{
			params ["_control"];
			_args params ["_logic"];
			deleteVehicle _logic;
			[_localId] call KH_fnc_removeEventHandler;
			nil;
		}
	] call KH_fnc_addEventHandler;
};

true;