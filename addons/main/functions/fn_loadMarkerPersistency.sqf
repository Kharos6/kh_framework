params [["_identifier", "", [""]], ["_overrideAttributes", [], [[]]]];
if (_identifier isEqualTo "") exitWith {};
private _markerPersistency = ([_identifier, "persistency"] joinString "_") readKhData [["markerPersistency", toLowerANSI worldName] joinString "_", createHashMap];
private _definedMarkers = [];

{
    private _attributes = _markerPersistency get _x;

    if !(isNil "_attributes") then {
        _definedMarkers pushBack _x;
        [_x, _attributes, _overrideAttributes, true] call KH_fnc_setMarkerAttributes;
    };
} forEach (keys KH_var_allMarkers);

{
    if (_x in _definedMarkers) then {
        continue;
    };

    private _sideUnit = KH_var_allPlayerUnits findIf {(side (group _x)) isEqualTo (_y select 1);};

    if ((_sideUnit isNotEqualTo -1) && ((_y select 1) isNotEqualTo sideUnknown)) then {
        [createMarker [_x, (_y select 4) select 1, _y select 2, KH_var_allPlayerUnits select _sideUnit], _y, [], false] call KH_fnc_setMarkerAttributes;
    }
    else {
        KH_var_queuedMarkers pushBackUnique [_x, _y select 1, _y select 2, (_y select 4) select 1, _y];
    };
} forEach _markerPersistency;

nil;