isNil {
	params [["_display", displayNull, [displayNull]], ["_logic", objNull, [objNull]]];

	[
		["CONTROL", _display displayCtrl 1],
		"ButtonClick",
		[_display, _logic],
		{
			_args params ["_display", "_logic"];
            private _fadeType = parseNumber (ctrlText (_display displayCtrl 102));

            if (_fadeType isEqualTo -2) then {
                _fadeType = false;
            }
            else {
                if (_fadeType isEqualTo -1) then {
                    _fadeType = true;
                };
            };

			[
				["KH_MissionConcluded", ctrlChecked (_display displayCtrl 101), _fadeType, 1, []],
				"KH_fnc_endMission",
				"SERVER",
				true,
				false
			] call KH_fnc_execute;

			deleteVehicle _logic;
			[_handlerId] call KH_fnc_removeHandler;
		}
	] call KH_fnc_addEventHandler;

	[
		["CONTROL", _display displayCtrl 2],
		"ButtonClick",
		[_logic],
		{
			_args params ["_logic"];
			deleteVehicle _logic;
			[_handlerId] call KH_fnc_removeHandler;
		}
	] call KH_fnc_addEventHandler;
};

nil;