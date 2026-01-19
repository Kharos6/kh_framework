params [["_state", true, [true]]];
KH_var_diagnosticsState = _state;
publicVariable "KH_var_diagnosticsState";

if !(isNil "KH_var_diagnosticsValidatePositionHelper") then {
	deleteVehicle KH_var_diagnosticsValidatePositionHelper;
	KH_var_diagnosticsValidatePositionHelper = nil;
};

if !(isNil "KH_var_diagnosticsDrawHandler") then {
	removeMissionEventHandler ["Draw3D", KH_var_diagnosticsDrawHandler];
	KH_var_diagnosticsDrawHandler = nil;
};

if !(isNil "KH_var_diagnosticsHandler") then {
	[KH_var_diagnosticsHandler] call KH_fnc_removeHandler;
	KH_var_diagnosticsHandler = nil;
};

if !(isNil "KH_var_diagnosticsAllMarkers") then {
	{
		deleteMarkerLocal _x;
	} forEach KH_var_diagnosticsAllMarkers;

	KH_var_diagnosticsAllMarkers resize 0;
};

if !(isNil "KH_var_diagnosticsCurrentMarkers") then {
	{
		deleteMarkerLocal _x;
	} forEach KH_var_diagnosticsCurrentMarkers;

	KH_var_diagnosticsCurrentMarkers resize 0;
};

if _state then {
	KH_var_diagnosticsAllMarkers = [];
	KH_var_diagnosticsInformation = [];
	KH_var_diagnosticsCurrentMarkers = [];
	KH_var_diagnosticsValidatePositionHelper = createSimpleObject ["KH_HelperRectangle_1x1x2", [0, 0, 0], true];
	KH_var_diagnosticsValidatePositionHelper setPhysicsCollisionFlag false;

	KH_var_diagnosticsDrawHandler = [
		"MISSION",
		"Draw3D",
		[],
		{	
			{
				_x params ["_unit", "_framerateOutput", "_framerateColor", "_viewDistance", "_cameraPosition", "_cameraDirection", "_playerUnit"];

				if (isNull curatorCamera) then {
					_framerateOutput = [name _unit, " - ", _framerateOutput] joinString "";
				}
				else {
					if (_unit isNotEqualTo _playerUnit) then {
						_framerateOutput = [name _unit, " - ", _framerateOutput] joinString "";
					};

					if (_playerUnit isNotEqualTo KH_var_playerUnit) then {
						private _mousePosition3d = ([KH_var_diagnosticsValidatePositionHelper] call KH_fnc_getMouseTarget) select 0;
						KH_var_diagnosticsValidatePositionHelper setPosASL _mousePosition3d;
						KH_var_diagnosticsValidatePositionHelper setVectorDirAndUp [[0, 1, 0], [0, 0, 1]];
						private _raycasts = [];

						if ([_playerUnit, _mousePosition3d, _cameraDirection, 1, 0, _viewDistance, false] call KH_fnc_getPositionVisibility) then {
							for "_positionX" from -0.5 to 0.5 step 0.5 do {
								for "_positionY" from -0.5 to 0.5 step 0.5 do {
									for "_positionZ" from 0 to 2 step 0.5 do {
										_raycasts pushBack [
											AGLToASL _cameraPosition,
											_mousePosition3d vectorAdd [_positionX, _positionY, _positionZ],
											[_playerUnit, objectParent _playerUnit, attachedTo _playerUnit] + (attachedObjects _playerUnit),
											true,
											1,
											"VIEW",
											"NONE",
											true,
											[]
										]
									};
								};
							};

							{
								if ((_x select 3) isEqualTo KH_var_diagnosticsValidatePositionHelper) then {
									private _mousePosition3dAgl = ASLToAGL _mousePosition3d;
									drawLine3D [_cameraPosition, _mousePosition3dAgl, [1, 0, 0, 0.9], 10];

									drawIcon3D [
										"\a3\ui_f\data\map\markers\military\warning_CA.paa",
										[1, 0, 0, 0.9],
										_mousePosition3dAgl,
										0.5,
										0.5,
										0,
										"",
										0,
										0.04,
										"PuristaMedium",
										"center",
										false
									];

									drawIcon3D [
										"\a3\ui_f\data\map\markers\military\warning_CA.paa",
										[1, 0, 0, 0.9],
										_cameraPosition,
										0.5,
										0.5,
										0,
										"",
										0,
										0.04,
										"PuristaMedium",
										"center",
										false
									];

									break;
								};
							} forEach ([_raycasts] call KH_fnc_raycast);
						};
					};
				};

				drawIcon3D [
					"",
					_framerateColor,
					_playerUnit modelToWorldVisual [0, 0, 0],
					1,
					2,
					0,
					_framerateOutput,
					2,
					0.04,
					"PuristaMedium",
					"center",
					true
				];
			} forEach KH_var_diagnosticsInformation;
		}
	] call KH_fnc_addEventHandler;

	KH_var_diagnosticsHandler = [
		[],
		{
			KH_var_diagnosticsInformation resize 0;
			KH_var_diagnosticsCurrentMarkers resize 0;
			KH_var_diagnosticsValidatePositionHelper hideObject (isNull curatorCamera);

			{
				private _framerate = _x getVariable ["KH_var_diagnosticsFramerate", 1];
				private _framerateOutput = ["FPS: ", _framerate] joinString "";
				private _unit = _x;
				
				private _framerateColor = switch true do {
					case ((_framerate < 30) && (_framerate >= 20)): {
						[1, 0.87, 0.12, 0.66];
					};
					
					case ((_framerate < 20) && (_framerate >= 10)): {
						[1, 0.64, 0, 0.66];
					};
					
					case (_framerate < 10): {
						[1, 0, 0, 0.66];
					};

					default {
						[0, 1, 0, 0.9];
					};
				};

				KH_var_diagnosticsInformation pushBack [
					_unit, 
					_framerateOutput, 
					_framerateColor, 
					_x getVariable ["KH_var_playerViewDistance", 1],
					_x getVariable ["KH_var_playerCameraPosition", eyePos _x],
					_x getVariable ["KH_var_playerCameraDirection", eyeDirection _x],
					_x getVariable ["KH_var_playerUnit", objNull]
				];
			} forEach (KH_var_allPlayerUnits + KH_var_allHeadlessUnits);

			private _serverFramerate = missionNamespace getVariable ["KH_var_diagnosticsFramerateServer", 1];
			private _worldX = (worldSize * 0.0033);
			private _worldY = (worldSize * 0.0065);
			private _worldYInterval = (_worldY * 2.45);
			private _serverOutput = ["SERVER - FPS: ", _serverFramerate, ", LOCAL UNITS: ", missionNamespace getVariable ["KH_var_diagnosticsLocalUnitsServer", 1]] joinString "";
			private _markerName = "KH_mrk_diagnosticsIdServer";
			KH_var_diagnosticsCurrentMarkers pushBackUnique _markerName;
			KH_var_diagnosticsAllMarkers pushBackUnique _markerName;
			
			private _markerColor = switch true do {
				case ((_serverFramerate < 40) && (_serverFramerate >= 30)): {
					"ColorYellow";
				};
				
				case ((_serverFramerate < 30) && (_serverFramerate >= 20)): {
					"ColorOrange";
				};
				
				case (_serverFramerate < 20): {
					"ColorRed";
				};

				default {
					"ColorGreen";
				};
			};

			createMarkerLocal [_markerName, [worldSize + _worldX, _worldY]];
			_markerName setMarkerPosLocal [worldSize + _worldX, _worldY];
			_markerName setMarkerTextLocal _serverOutput;
			_markerName setMarkerColorLocal _markerColor;
			_markerName setMarkerShapeLocal "ICON";
			_markerName setMarkerTypeLocal "mil_dot";
			_markerName setMarkerSizeLocal [0, 0];
			
			{
				_worldY = _worldY + _worldYInterval;
				private _output = "";
				private _framerate = _x getVariable ["KH_var_diagnosticsFramerate", 1];
				
				if !(_x isNil "KH_var_diagnosticsLocalUnits") then {
					_output = [name _x, " - FPS: ", _framerate, ", LOCAL UNITS: ", _x getVariable "KH_var_diagnosticsLocalUnits"] joinString "";
				}
				else {
					_output = [name _x, " - FPS: ", _framerate] joinString "";
				};
				
				private _markerName = ["KH_mrk_diagnosticsId", getPlayerID _x] joinString "";
				KH_var_diagnosticsCurrentMarkers pushBackUnique _markerName;
				KH_var_diagnosticsAllMarkers pushBackUnique _markerName;

				private _markerColor = switch true do {
					case ((_framerate < 40) && (_framerate >= 30)): {
						"ColorYellow";
					};
					
					case ((_framerate < 30) && (_framerate >= 20)): {
						"ColorOrange";
					};
					
					case (_framerate < 20): {
						"ColorRed";
					};

					default {
						"ColorGreen";
					};
				};

				createMarkerLocal [_markerName, [worldSize + _worldX, _worldY]];
				_markerName setMarkerPosLocal [worldSize + _worldX, _worldY];
				_markerName setMarkerTextLocal _output;
				_markerName setMarkerColorLocal _markerColor;
				_markerName setMarkerShapeLocal "ICON";
				_markerName setMarkerTypeLocal "mil_dot";
				_markerName setMarkerSizeLocal [0, 0];
			} forEach (KH_var_allPlayerUnits + KH_var_allHeadlessUnits);

			[
				KH_var_diagnosticsAllMarkers,
				{
					if !(_x in KH_var_diagnosticsCurrentMarkers) then {
						deleteMarkerLocal _x;
						true;
					};
				}
			] call KH_fnc_deleteArrayElements;
		},
		true, 
		0,
		false
	] call KH_fnc_execute;
};

nil;