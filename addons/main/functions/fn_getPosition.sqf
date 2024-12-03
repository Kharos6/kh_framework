params ["_entity", ["_positionType", "AEL"], ["_relativePosition", []]];

switch true do {
	case (_positionType == "ATL"): {
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

	case (_positionType == "ASL"): {
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

	case (_positionType == "ASLW"): {
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

	case (_positionType == "AEL"): {
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

	case (_positionType == "AGL"): {
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

	case (_positionType == "AGLS"): {
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

	case (_positionType == "WORLD"): {
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