params [["_state", true, [true]]];
KH_var_diagnosticsState = _state;
publicVariable "KH_var_diagnosticsState";

if !(isNil "KH_var_diagnosticsDrawHandler") then {
	removeMissionEventHandler ["Draw3D", KH_var_diagnosticsDrawHandler];
};

if !(isNil "KH_var_diagnosticsHandler") then {
	[KH_var_diagnosticsHandler] call KH_fnc_removeHandler;
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

	KH_var_diagnosticsDrawHandler = addMissionEventHandler [
		"Draw3D",
		{	
			{
				_x params ["_output", "_color", "_unit"];
				
				if (isNull curatorCamera) then {
					_output = [name _unit, " - ", _output] joinString "";
				};

				drawIcon3D [
					"",
					_color,
					_unit modelToWorldVisual [0, 0, 0],
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
			} forEach KH_var_diagnosticsInformation;
		}
	];

	KH_var_diagnosticsHandler = [
		[],
		{
			KH_var_diagnosticsInformation resize 0;

			{
				private _currentInformation = [];
				private _framerate = _x getVariable ["KH_var_diagnosticsFramerate", 1];
				private _output = ["FPS: ", _framerate] joinString "";
				private _color = [0, 1, 0, 0.9];
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
				KH_var_diagnosticsInformation pushBack _currentInformation;
			} forEach (KH_var_allPlayerUnits + KH_var_allHeadlessUnits);

			private _worldX = (worldSize * 0.0033);
			private _worldY = (worldSize * 0.0065);
			private _worldYInterval = (_worldY * 2.45);
			private _serverOutput = ["SERVER - FPS: ", missionNamespace getVariable ["KH_var_diagnosticsFramerateServer", 1], ", LOCAL UNITS: ", missionNamespace getVariable ["KH_var_diagnosticsLocalUnitsServer", 1]] joinString "";
			private _markerName = "KH_mrk_diagnosticsIdServer";
			KH_var_diagnosticsCurrentMarkers pushBackUnique _markerName;
			KH_var_diagnosticsAllMarkers pushBackUnique _markerName;
			
			private _markerColor = switch true do {
				case ((diag_fps < 40) && (diag_fps >= 30)): {
					"ColorYellow";
				};
				
				case ((diag_fps < 30) && (diag_fps >= 20)): {
					"ColorOrange";
				};
				
				case (diag_fps < 20): {
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
				
				if !(isNil {_x getVariable "KH_var_diagnosticsLocalUnits";}) then {
					_output = [name _x, " - FPS: ", _framerate, ", LOCAL UNITS: ", _x getVariable ["KH_var_diagnosticsLocalUnits", 1]] joinString "";
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
		1,
		false
	] call KH_fnc_execute;
};