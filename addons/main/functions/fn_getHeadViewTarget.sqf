params [["_ignored", [], [[], objNull]]];

_ignored = if (_ignored isEqualType objNull) then {
    if !(isNull _ignored) then {
        [_ignored] + [KH_var_playerUnit, objectParent KH_var_playerUnit, attachedTo KH_var_playerUnit] + (attachedObjects KH_var_playerUnit);
    }
    else {
        [KH_var_playerUnit, objectParent KH_var_playerUnit, attachedTo KH_var_playerUnit] + (attachedObjects KH_var_playerUnit);
    }
}
else {
    _ignored + [KH_var_playerUnit, objectParent KH_var_playerUnit, attachedTo KH_var_playerUnit] + (attachedObjects KH_var_playerUnit);
};

if ((KH_var_headViewTargetCheckFrame isEqualTo diag_frameNo) && (KH_var_headViewTargetIgnores isEqualTo _ignored)) exitWith {
    [
        KH_var_headViewTargetSurfacePosition,
        KH_var_headViewTargetSurfaceDistance,
        KH_var_headViewTargetSurfaceNormal,
        KH_var_headViewTargetObject,
        KH_var_headViewTargetParentObject,
        KH_var_headViewTargetSelections,
        KH_var_headViewTargetSurfaceType,
        KH_var_headViewTargetDistance
    ];
};

KH_var_headViewTargetIgnores = _ignored;
KH_var_headViewTargetCheckFrame = diag_frameNo;
private _viewPosition = KH_var_playerUnit modelToWorldWorld (KH_var_playerUnit selectionPosition "head");

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

KH_var_headViewTargetSurfacePosition = _viewTarget param [0, _viewPosition];
KH_var_headViewTargetSurfaceDistance = _viewPosition vectorDistance KH_var_headViewTargetSurfacePosition;
KH_var_headViewTargetSurfaceNormal = _viewTarget param [1, []];
KH_var_headViewTargetObject = _viewTarget param [2, objNull];
KH_var_headViewTargetParentObject = _viewTarget param [3, objNull];
KH_var_headViewTargetSelections = _viewTarget param [4, []];
KH_var_headViewTargetSurfaceType = _viewTarget param [5, ""];

KH_var_headViewTargetDistance = if !(isNull KH_var_headViewTargetObject) then {
    (getPosASL KH_var_playerUnit) vectorDistance (getPosASL KH_var_headViewTargetObject);
}
else {
    _viewPosition vectorDistance KH_var_headViewTargetSurfacePosition;
};

[
    KH_var_headViewTargetSurfacePosition,
    KH_var_headViewTargetSurfaceDistance,
    KH_var_headViewTargetSurfaceNormal,
    KH_var_headViewTargetObject,
    KH_var_headViewTargetParentObject,
    KH_var_headViewTargetSelections,
    KH_var_headViewTargetSurfaceType,
    KH_var_headViewTargetDistance
];