params [["_state", true], ["_3dMode", false], ["_ownership", false], ["_scheduler", false]];
KH_var_diagnostics3dMode = _3dMode;
publicVariable "KH_var_diagnostics3dMode";
KH_var_diagnosticsOwnership = _ownership;
publicVariable "KH_var_diagnosticsOwnership";
KH_var_diagnosticsScheduler = _scheduler;
publicVariable "KH_var_diagnosticsScheduler";

if _state then {
	KH_var_diagnosticsState = true;
	publicVariable "KH_var_diagnosticsState";

	if (isNil "KH_var_diagnosticsSet") then {
		KH_var_diagnosticsSet = true;
		KH_var_diagnosticsAllMarkers = [];	
		
		[
			[], 
			{
				if !isDedicated then {
					[
						{
							if KH_var_diagnosticsState then {
								player setVariable ["KH_var_diagnosticsFramerate", diag_fps, 2];
								
								if KH_var_diagnosticsScheduler then {
									player setVariable ["KH_var_diagnosticsActiveScripts", diag_activeScripts, 2];
								};
							};
						},
						1, 
						[]
					] call CBA_fnc_addPerFrameHandler;
					
					if !hasInterface then {
						[
							{
								if KH_var_diagnosticsState then {
									player setVariable ["KH_var_diagnosticsLocalUnits", {local _x;} count allUnits, 2];
								};
							},
							1, 
							[]
						] call CBA_fnc_addPerFrameHandler;
					};
				};
			},
			["JIP", "GLOBAL", objNull, true, false],
			"THIS_FRAME"
		] call KH_fnc_execute;

		private _adminFunction = {
			[
				[], 
				{
					addMissionEventHandler [
						"Draw3D",
						{
							if (KH_var_currentAdmin == clientOwner) then {
								if KH_var_diagnosticsState then {
									if KH_var_diagnostics3dMode then {		
										{
											_x params ["_output", "_color", "_unit"];
											
											if (isNull curatorCamera) then {
												_output = format ["%1 - %2", name _unit, _output];
											};

											drawIcon3D
											[
												"",
												_color,
												_unit modelToWorld [0, 0, 0],
												1,
												2,
												0,
												_output,
												2,
												0.03,
												"PuristaMedium",
												"center",
												true
											];
										} forEach (missionNamespace getVariable ["KH_var_diagnosticsInformation", []]);
									};
									
									if KH_var_diagnosticsOwnership then {
										{
											_x params ["_output", "_unit"];

											drawIcon3D
											[
												"",
												[1, 1, 1, 0.66],
												_unit modelToWorld [0, 0, 0],
												1,
												2,
												0,
												_output,
												2,
												0.03,
												"PuristaMedium",
												"center",
												true
											];
										} forEach (missionNamespace getVariable ["KH_var_diagnosticsOwnershipInformation", []]);
									};
								};
							}
							else {
								{
									deleteMarkerLocal _x;
								} forEach KH_var_diagnosticsMarkers;
								
								removeMissionEventHandler [_thisEvent, _thisEventHandler];
							};
						}
					];
				},
				KH_var_currentAdmin,
				"THIS_FRAME"
			] call KH_fnc_execute;
		};

		[] call _adminFunction;

		[
			["CBA"],
			"KH_eve_adminChanged",
			[_adminFunction],
			{
				_args params ["_adminFunction"];
				[] call _adminFunction;
			}
		] call KH_fnc_addEventHandler;
		
		[
			{		
				if KH_var_diagnosticsState then {
					if KH_var_diagnostics3dMode then {
						private _information = [];

						{
							private _currentInformation = [];
							private _framerate = floor (_x getVariable ["KH_var_diagnosticsFramerate", 1]);
							private _output = format ["FPS: %1", _framerate];
							private _color = [0, 1, 0, 0.66];
							private _unit = _x;
							
							switch true do {
								case ((_framerate < 30) && (_framerate >= 20)): {
									_color = [1, 0.87, 0.12, 0.66];
								};
								
								case ((_framerate < 20) && (_framerate >= 10)): {
									_color = [1, 0.64, 0, 0.66];
								};
								
								case (_framerate < 10): {
									_color = [1, 0, 0, 0.66];
								};
							};

							_currentInformation pushBack _output;
							_currentInformation pushBack _color;
							_currentInformation pushBack _unit;
							_information pushBack _currentInformation;
						} forEach KH_var_allPlayerUnits;

						missionNamespace setVariable ["KH_var_diagnosticsInformation", _information, KH_var_currentAdmin];
					};

					if KH_var_diagnosticsOwnership then {
						private _information = [];

						{
							private _currentInformation = [];
							private _output = format ["OWNER ID: %1", owner _x];
							private _unit = _x;
							_currentInformation pushBack _output;
							_currentInformation pushBack _unit;
							_information pushBack _currentInformation;						
						} forEach ([["ALL"], false, false] call KH_fnc_getUnits);

						missionNamespace setVariable ["KH_var_diagnosticsOwnershipInformation", _information, KH_var_currentAdmin];
					};

					KH_var_diagnosticsMarkers = [];
					KH_var_currentAdmin publicVariableClient "KH_var_diagnosticsMarkers";
					private _worldX = (worldSize * 0.0033);
					private _worldY = (worldSize * 0.0065);
					private _worldYInterval = (_worldY * 2.2);
					private _output = "";

					if KH_var_diagnosticsScheduler then {
						_output = format ["SERVER - FPS: %1, LOCAL UNITS: %2, ACTIVE SCRIPTS: %3", floor diag_fps, {local _x;} count allUnits, diag_activeScripts];
					}
					else {
						_output = format ["SERVER - FPS: %1, LOCAL UNITS: %2", floor diag_fps, {local _x;} count allUnits];
					};

					private _markerName = "KH_mrk_diagnosticsId2";
					KH_var_diagnosticsMarkers pushBackUnique _markerName;
					KH_var_currentAdmin publicVariableClient "KH_var_diagnosticsMarkers";
					KH_var_diagnosticsAllMarkers pushBackUnique _markerName;
					private _markerColor = "ColorGreen";
					
					switch true do {
						case ((diag_fps < 40) && (diag_fps >= 30)): {
							_markerColor = "ColorYellow";
						};
						
						case ((diag_fps < 30) && (diag_fps >= 20)): {
							_markerColor = "ColorOrange";
						};
						
						case (diag_fps < 20): {
							_markerColor = "ColorRed";
						};
					};
					
					[
						[_worldY, _worldX, _output, _markerName, _markerColor], 
						{
							params ["_worldY", "_worldX", "_output", "_markerName", "_markerColor"];
							createMarkerLocal [_markerName, [worldSize + _worldX, _worldY]];
							_markerName setMarkerPosLocal [worldSize + _worldX, _worldY];
							_markerName setMarkerTextLocal _output;
							_markerName setMarkerColorLocal _markerColor;
							_markerName setMarkerShapeLocal "ICON";
							_markerName setMarkerTypeLocal "mil_dot";
							_markerName setMarkerSizeLocal [0, 0];
						},
						KH_var_currentAdmin,
						"THIS_FRAME"
					] call KH_fnc_execute;
					
					{
						_worldY = _worldY + _worldYInterval;
						private _output = "";
						
						if ((getPlayerID _x) getUserInfo 7) then {
							if KH_var_diagnosticsScheduler then {
								_output = format ["%1 (ID %2) - FPS: %3, LOCAL UNITS: %4, ACTIVE SCRIPTS: %5", name _x, owner _x, floor (_x getVariable ["KH_var_diagnosticsFramerate", 1]), _x getVariable ["KH_var_diagnosticsLocalUnits", 1], _x getVariable ["KH_var_diagnosticsActiveScripts", []]];
							}
							else {
								_output = format ["%1 (ID %2) - FPS: %3, LOCAL UNITS: %4", name _x, owner _x, floor (_x getVariable ["KH_var_diagnosticsFramerate", 1]), _x getVariable ["KH_var_diagnosticsLocalUnits", 1]];
							};
						}
						else {
							if KH_var_diagnosticsScheduler then {
								_output = format ["%1 (ID %2) - FPS: %3, ACTIVE SCRIPTS: %4", name _x, owner _x, floor (_x getVariable ["KH_var_diagnosticsFramerate", 1]), _x getVariable ["KH_var_diagnosticsActiveScripts", []]];
							}
							else {
								_output = format ["%1 (ID %2) - FPS: %3", name _x, owner _x, floor (_x getVariable ["KH_var_diagnosticsFramerate", 1])];
							};
						};
						
						private _markerName = format ["KH_mrk_diagnosticsId%1", owner _x];
						KH_var_diagnosticsMarkers pushBackUnique _markerName;
						KH_var_currentAdmin publicVariableClient "KH_var_diagnosticsMarkers";
						KH_var_diagnosticsAllMarkers pushBackUnique _markerName;
						private _markerColor = "ColorGreen";
						
						switch true do {
							case ((_framerate < 40) && (_framerate >= 30)): {
								_markerColor = "ColorYellow";
							};
							
							case ((_framerate < 30) && (_framerate >= 20)): {
								_markerColor = "ColorOrange";
							};
							
							case (_framerate < 20): {
								_markerColor = "ColorRed";
							};
						};
						
						[
							[_worldY, _worldX, _output, _markerName, _markerColor], 
							{
								params ["_worldY", "_worldX", "_output", "_markerName", "_markerColor"];
								createMarkerLocal [_markerName, [worldSize + _worldX, _worldY]];
								_markerName setMarkerPosLocal [worldSize + _worldX, _worldY];
								_markerName setMarkerTextLocal _output;
								_markerName setMarkerColorLocal _markerColor;
								_markerName setMarkerShapeLocal "ICON";
								_markerName setMarkerTypeLocal "mil_dot";
								_markerName setMarkerSizeLocal [0, 0];
							},
							KH_var_currentAdmin,
							"THIS_FRAME"
						] call KH_fnc_execute;
					} forEach ([["PLAYERS", "HEADLESS"], true] call KH_fnc_getClients);
					
					private _markerCheck = [];
					
					{
						if !(_x in KH_var_diagnosticsMarkers) then {
							_markerCheck pushBackUnique _x;
							
							[
								[_x], 
								{
									params ["_marker"];
									deleteMarkerLocal _marker;
								},
								KH_var_currentAdmin,
								"THIS_FRAME"
							] call KH_fnc_execute;
						};
					} forEach KH_var_diagnosticsAllMarkers;
					
					{
						if (_x in KH_var_diagnosticsAllMarkers) then {
							KH_var_diagnosticsAllMarkers deleteAt (KH_var_diagnosticsAllMarkers find _x);
						};
					} forEach _markerCheck;
				}
				else {
					[
						[], 
						{						
							{
								deleteMarkerLocal _x;
							} forEach KH_var_diagnosticsMarkers;
						},
						KH_var_currentAdmin,
						"THIS_FRAME"
					] call KH_fnc_execute;
				};
			},
			1, 
			[]
		] call CBA_fnc_addPerFrameHandler;
	};
}
else {
	KH_var_diagnosticsState = false;
	publicVariable "KH_var_diagnosticsState";	
};

true;