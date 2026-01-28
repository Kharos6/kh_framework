params [["_ignored", [], [[], objNull]]];

_ignored = if (_ignored isEqualType objNull) then {
    if !(isNull _ignored) then {
        [_ignored] + [KH_var_playerUnit, objectParent KH_var_playerUnit, attachedTo KH_var_playerUnit] + (attachedObjects KH_var_playerUnit);
    }
    else {
        [KH_var_playerUnit, objectParent KH_var_playerUnit, attachedTo KH_var_playerUnit] + (attachedObjects KH_var_playerUnit);
    };
}
else {
    _ignored + [KH_var_playerUnit, objectParent KH_var_playerUnit, attachedTo KH_var_playerUnit] + (attachedObjects KH_var_playerUnit);
};

if ((KH_var_viewTargetCheckFrame isEqualTo diag_frameNo) && (KH_var_viewTargetIgnores isEqualTo _ignored)) exitWith {
    [
        KH_var_viewTargetSurfacePosition,
        KH_var_viewTargetSurfaceDistance,
        KH_var_viewTargetSurfaceNormal,
        KH_var_viewTargetObject,
        KH_var_viewTargetParentObject,
        KH_var_viewTargetSelections,
        KH_var_viewTargetSurfaceType,
        KH_var_viewTargetDistance
    ];
};

KH_var_viewTargetIgnores = _ignored;
KH_var_viewTargetCheckFrame = diag_frameNo;
private _viewPosition = AGLToASL (positionCameraToWorld [0, 0, 0]);

private _viewTarget = ([
    _viewPosition, 
    _viewPosition vectorAdd ((getCameraViewDirection KH_var_playerUnit) vectorMultiply viewDistance), 
    _ignored,
    true, 
    1, 
    "VIEW", 
    "FIRE", 
    true,
    []
] call KH_fnc_raycast) param [0, []];

KH_var_viewTargetSurfacePosition = _viewTarget param [0, _viewPosition];
KH_var_viewTargetSurfaceDistance = _viewPosition vectorDistance KH_var_viewTargetSurfacePosition;
KH_var_viewTargetSurfaceNormal = _viewTarget param [1, []];
KH_var_viewTargetObject = _viewTarget param [2, objNull];
KH_var_viewTargetParentObject = _viewTarget param [3, objNull];
KH_var_viewTargetSelections = _viewTarget param [4, []];
KH_var_viewTargetSurfaceType = _viewTarget param [5, ""];

KH_var_viewTargetDistance = if !(isNull KH_var_viewTargetObject) then {
    (getPosASL KH_var_playerUnit) vectorDistance (getPosASL KH_var_viewTargetObject);
}
else {
    _viewPosition vectorDistance KH_var_viewTargetSurfacePosition;
};

[
    KH_var_viewTargetSurfacePosition,
    KH_var_viewTargetSurfaceDistance,
    KH_var_viewTargetSurfaceNormal,
    KH_var_viewTargetObject,
    KH_var_viewTargetParentObject,
    KH_var_viewTargetSelections,
    KH_var_viewTargetSurfaceType,
    KH_var_viewTargetDistance
];