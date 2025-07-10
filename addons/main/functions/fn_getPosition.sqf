params ["_entity", ["_positionType", "AEL"], ["_relativePosition", []]];

switch _positionType do {
	case "ATL": {
		if (_relativePosition isEqualTo []) then {
			getPosATL _entity;
		}
		else {
			private _offsetPosition = (_entity modelToWorld _relativePosition);
			private _positionMarker = createVehicleLocal ["KH_Marker_Square", _offsetPosition, [], 0, "CAN_COLLIDE"];
			[_positionMarker, _offsetPosition, "AGL", false] call KH_fnc_setPosition;
			private _position = getPosATL _positionMarker;
			deleteVehicle _positionMarker;
			_position;
		};
	};

	case "ASL": {
		if (_relativePosition isEqualTo []) then {
			getPosASL _entity;
		}
		else {
			private _offsetPosition = (_entity modelToWorld _relativePosition);
			private _positionMarker = createVehicleLocal ["KH_Marker_Square", _offsetPosition, [], 0, "CAN_COLLIDE"];
			[_positionMarker, _offsetPosition, "AGL", false] call KH_fnc_setPosition;
			private _position = getPosASL _positionMarker;
			deleteVehicle _positionMarker;
			_position;
		};
	};

	case "ASLW": {
		if (_relativePosition isEqualTo []) then {
			getPosASLW _entity;
		}
		else { 
			private _offsetPosition = (_entity modelToWorld _relativePosition);
			private _positionMarker = createVehicleLocal ["KH_Marker_Square", _offsetPosition, [], 0, "CAN_COLLIDE"];
			[_positionMarker, _offsetPosition, "AGL", false] call KH_fnc_setPosition;
			private _position = getPosASLW _positionMarker;
			deleteVehicle _positionMarker;
			_position;
		};
	};

	case "AEL": {
		if (surfaceIsWater (getPosWorld _entity)) then {
			if (_relativePosition isEqualTo []) then {
				getPosASL _entity;
			}
			else {
				private _offsetPosition = (_entity modelToWorld _relativePosition);
				private _positionMarker = createVehicleLocal ["KH_Marker_Square", _offsetPosition, [], 0, "CAN_COLLIDE"];
				[_positionMarker, _offsetPosition, "AGL", false] call KH_fnc_setPosition;
				private _position = getPosASL _positionMarker;
				deleteVehicle _positionMarker;
				_position;
			};
		}
		else {
			if (_relativePosition isEqualTo []) then {
				getPosATL _entity;
			}
			else {
				private _offsetPosition = (_entity modelToWorld _relativePosition);
				private _positionMarker = createVehicleLocal ["KH_Marker_Square", _offsetPosition, [], 0, "CAN_COLLIDE"];
				[_positionMarker, _offsetPosition, "AGL", false] call KH_fnc_setPosition;
				private _position = getPosATL _positionMarker;
				deleteVehicle _positionMarker;
				_position;
			};
		};
	};

	case "AGL": {
		if (surfaceIsWater (getPosWorld _entity)) then {
			if (_relativePosition isEqualTo []) then {
				getPosASLW _entity;
			}
			else { 
				private _offsetPosition = (_entity modelToWorld _relativePosition);
				private _positionMarker = createVehicleLocal ["KH_Marker_Square", _offsetPosition, [], 0, "CAN_COLLIDE"];
				[_positionMarker, _offsetPosition, "AGL", false] call KH_fnc_setPosition;
				private _position = getPosASLW _positionMarker;
				deleteVehicle _positionMarker;
				_position;
			};
		}
		else {
			if (_relativePosition isEqualTo []) then {
				getPosATL _entity;
			}
			else {
				private _offsetPosition = (_entity modelToWorld _relativePosition);
				private _positionMarker = createVehicleLocal ["KH_Marker_Square", _offsetPosition, [], 0, "CAN_COLLIDE"];
				[_positionMarker, _offsetPosition, "AGL", false] call KH_fnc_setPosition;
				private _position = getPosATL _positionMarker;
				deleteVehicle _positionMarker;
				_position;
			};
		};
	};

	case "AGLS": {
		if (_relativePosition isEqualTo []) then {
			getPos _entity;
		}
		else {
			private _offsetPosition = (_entity modelToWorld _relativePosition);
			private _positionMarker = createVehicleLocal ["KH_Marker_Square", _offsetPosition, [], 0, "CAN_COLLIDE"];
			[_positionMarker, _offsetPosition, "AGL", false] call KH_fnc_setPosition;
			private _position = getPos _positionMarker;
			deleteVehicle _positionMarker;
			_position;
		};
	};

	case "WORLD": {
		if (_relativePosition isEqualTo []) then {
			getPosWorld _entity;
		}
		else {
			private _offsetPosition = (_entity modelToWorld _relativePosition);
			private _positionMarker = createVehicleLocal ["KH_Marker_Square", _offsetPosition, [], 0, "CAN_COLLIDE"];
			[_positionMarker, _offsetPosition, "AGL", false] call KH_fnc_setPosition;
			private _position = getPosWorld _positionMarker;
			deleteVehicle _positionMarker;
			_position;
		};
	};

	default {
		[];
	};
};