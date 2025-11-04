isNil {
	params [["_display", displayNull, [displayNull]], ["_logic", objNull, [objNull]]];

	[
		["CONTROL", _display displayCtrl 1],
		"ButtonClick",
		[_display, _logic],
		{
			_args params ["_display", "_logic"];

            if !KH_var_missionSuspended then {
                [
                    [],
                    {
                        {
                            _x enableSimulationGlobal false;
                            (objectParent _x) enableSimulationGlobal false;
                        } forEach ([KH_var_allPlayerUnits, allUnits] select (cbChecked (_display displayCtrl 101)));
                    },
                    "SERVER",
                    true,
                    false
                ] call KH_fnc_execute;

                [
                    [ctrlText (_display displayCtrl 102)],
                    {
                        params ["_suspensionText"];

                        with uiNamespace do {
                            if (isNil "KH_var_suspensionDisplay") then {
                                KH_var_suspensionDisplay = ["RscText", _suspensionText, [0, false, 0], [0, 0, 0, 1], [0, 0, 100, 100], false, [0, 0, 0]] call KH_fnc_draw2d;
                            }
                            else {
                                if (isNull KH_var_suspensionDisplay) then {
                                    KH_var_suspensionDisplay = ["RscText", _suspensionText, [0, false, 0], [0, 0, 0, 1], [0, 0, 100, 100], false, [0, 0, 0]] call KH_fnc_draw2d;
                                };
                            };
                        };
                    },
                    KH_var_allPlayerMachines - [KH_var_adminMachine, clientOwner],
                    true,
                    false
                ] call KH_fnc_execute;
            }
            else {
                [
                    [],
                    {
                        {
                            _x enableSimulationGlobal true;
                            (objectParent _x) enableSimulationGlobal true;
                        } forEach ([KH_var_allPlayerUnits, allUnits] select (cbChecked (_display displayCtrl 102)));
                    },
                    "SERVER",
                    true,
                    false
                ] call KH_fnc_execute;

                [
                    [],
                    {
                        with uiNamespace do {
                            if !(isNil "KH_var_suspensionDisplay") then {
                                KH_var_suspensionDisplay closeDisplay 2;
                                KH_var_suspensionDisplay = nil;
                            };
                        };
                    },
                    "PLAYERS",
                    true,
                    false
                ] call KH_fnc_execute;
            };

            KH_var_missionSuspended = !KH_var_missionSuspended;
            publicVariable "KH_var_missionSuspended";
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