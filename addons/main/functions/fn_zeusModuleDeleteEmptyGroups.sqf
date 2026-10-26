isNil {
	params [["_display", displayNull, [displayNull]], ["_logic", objNull, [objNull]]];

	[
		["CONTROL", _display displayCtrl 1],
		"ButtonClick",
		[_display, _logic],
		{
			_args params ["_display", "_logic"];

			if (cbChecked (_display displayCtrl 101)) then {
                execute [
                    [],
                    {
                        {
                            deleteGroup _x;
                        } forEach (allGroups select {((units _x) isEqualTo []) && (local _x);});
                    },
                    "GLOBAL",
                    true,
                    false
                ];
            }
            else {
                {
                    deleteGroup _x;
                } forEach (allGroups select {((units _x) isEqualTo []) && (local _x);});
            };
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