params [["_marker", "", [""]], ["_attributes", [], [[]]], ["_overrideAttributes", [], [[]]], ["_respectLocality", true, [true]]];
(_this select [1]) params ["_side", "_channel", "_userDefined", "_savedAttributes"];
private _continue = true;

if _respectLocality then {
	if ((((KH_var_allMarkers get _marker) param [0, sideUnknown]) isNotEqualTo _side) || (_userDefined && !("_USER_DEFINED" in _marker)) || (_channel isNotEqualTo (markerChannel _marker))) then {
		deleteMarker _marker;

		if (_userDefined && !("_USER_DEFINED" in _marker)) then {
			_marker = ["_USER_DEFINED ", _marker] joinString "";
		};

		private _sideUnit = KH_var_allPlayerUnits findIf {(side (group _x)) isEqualTo _side;};

		if ((_sideUnit isNotEqualTo -1) && (_side isNotEqualTo sideUnknown)) then {
			[createMarker [_marker, _savedAttributes select 1, _channel, KH_var_allPlayerUnits select _sideUnit], _savedAttributes, [], false] call KH_fnc_setMarkerAttributes;
		}
		else {
			KH_var_queuedMarkers pushBackUnique [_marker, _side, _channel, _savedAttributes select 1, _savedAttributes];
		};

		_continue = false;
	};
};

if !_continue exitWith {};

if (_overrideAttributes isNotEqualTo []) then {
	private _currentAttributes = ([_marker] call KH_fnc_getMarkerAttributes) select 4;

	{
		if (_x isEqualType 0) then {
			_savedAttributes set [_x, _currentAttributes select _x];
		}
		else {
			_x params [["_index", 0, [0]], "_value"];
			_savedAttributes set [_index, _value];
		};
	} forEach _overrideAttributes;
};

_marker setMarkerTypeLocal _type;
_marker setMarkerPosLocal _position;
_marker setMarkerDirLocal _direction;
_marker setMarkerSizeLocal _size;
_marker setMarkerColorLocal _color;
_marker setMarkerAlphaLocal _alpha;
_marker setMarkerTextLocal _text;
_marker setMarkerBrushLocal _brush;
_marker setMarkerPolylineLocal _polyline;
_marker setMarkerShadowLocal _shadow;
_marker setMarkerShape _shape;
nil;