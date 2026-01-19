isNil {
	params [["_display", displayNull, [displayNull]], ["_logic", objNull, [objNull]]];

	[
		["CONTROL", _display displayCtrl 1],
		"ButtonClick",
		[_display, _logic],
		{
			_args params ["_display"];

            [
                [cbChecked (_display displayCtrl 101), ctrlText (_display displayCtrl 102), clientOwner],
                {
                    params ["_unitType", "_suspensionText", "_caller"];

                    if !KH_var_missionSuspended then {
                        KH_var_missionSuspended = true;
                        publicVariable "KH_var_missionSuspended";
                        KH_var_missionSuspensionEntities = [];

                        {
                            if !(_x in KH_var_missionSuspensionEntities) then {
                                _x setVariable ["KH_var_originalSimulationState", simulationEnabled _x];
                                _x enableSimulationGlobal false;
                            };
                            
                            private _parent = objectParent _x;

                            if !(_parent in KH_var_missionSuspensionEntities) then {
                                _parent setVariable ["KH_var_originalSimulationState", simulationEnabled _parent];
                                _parent enableSimulationGlobal false;
                            };
                            
                            KH_var_missionSuspensionEntities insert [-1, [_x, _parent], true];
                        } forEach ([KH_var_allPlayerUnits, allUnits] select _unitType);

                        private _playerLoadHandler = [
                            "CBA",
                            "KH_eve_playerLoaded",
                            [],
                            {
                                private _unit = param [3];

                                if !(_unit in KH_var_missionSuspensionEntities) then {
                                    _unit setVariable ["KH_var_originalSimulationState", simulationEnabled _unit];
                                    _unit enableSimulationGlobal false;
                                };

                                private _parent = objectParent _unit;

                                if !(_parent in KH_var_missionSuspensionEntities) then {
                                    _parent setVariable ["KH_var_originalSimulationState", simulationEnabled _parent];
                                    _parent enableSimulationGlobal false;
                                };
                            
                                KH_var_missionSuspensionEntities insert [-1, [_unit, _parent], true];
                            }
                        ] call KH_fnc_addEventHandler;

                        [
                            [_playerLoadHandler],
                            {
                                params ["_playerLoadHandler"];
                                [_playerLoadHandler] call KH_fnc_removeHandler;
                            },
                            true,
                            {!KH_var_missionSuspended;},
                            false
                        ] call KH_fnc_execute;

                        [
                            [_suspensionText],
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
                            KH_var_allPlayerMachines - [KH_var_adminMachine, _caller],
                            true,
                            false
                        ] call KH_fnc_execute;
                    }
                    else {
                        KH_var_missionSuspended = false;
                        publicVariable "KH_var_missionSuspended";

                        {
                            _x enableSimulationGlobal (_x getVariable ["KH_var_originalSimulationState", true]);
                        } forEach KH_var_missionSuspensionEntities;

                        [
                            [],
                            {
                                with uiNamespace do {
                                    if !(isNil "KH_var_suspensionDisplay") then {
                                        ctrlDelete KH_var_suspensionDisplay;
                                        KH_var_suspensionDisplay = nil;
                                    };
                                };
                            },
                            "PLAYERS",
                            true,
                            false
                        ] call KH_fnc_execute;
                    };
                },
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