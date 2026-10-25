params [["_marker", "", [""]]];

[
	KH_var_serverMissionSessionId,
    (KH_var_allMarkers get _marker) param [0, sideUnknown],
    markerChannel _marker,
    "_USER_DEFINED" in _marker,
	[
        getMarkerType _marker,
        getMarkerPos _marker,
        markerDir _marker,
        getMarkerSize _marker,
        getMarkerColor _marker,
        markerAlpha _marker,
        markerText _marker,
        markerBrush _marker,
        markerDrawPriority _marker,
        markerPolyline _marker,
        markerShadow _marker,
        markerShape _marker
    ]
];