params [["_identifier", "", [""]], ["_markers", keys KH_var_allMarkers, []]];

if (_identifier isEqualTo "") exitWith {
	createHashMap;
};

private _persistencyNamespace = [_identifier, "persistency"] joinString "_";
private _persistencyVariable = ["markerPersistency", toLowerANSI worldName] joinString "_";
private _currentEntries = _persistencyNamespace readKhData [_persistencyVariable, createHashMap];

{
    private _marker = _x;

    if ("_USER_DEFINED" in _marker) then {
        _marker = ["_USER_DEFINED ", generateUid] joinString "";
    };

	_currentEntries set [_marker, [_x] call KH_fnc_getMarkerAttributes];
} forEach _markers;

_persistencyNamespace writeKhData [_persistencyVariable, _currentEntries];
_currentEntries;