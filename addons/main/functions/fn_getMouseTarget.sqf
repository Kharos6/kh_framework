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

if ((KH_var_mouseTargetCheckFrame isEqualTo diag_frameNo) && (KH_var_mouseTargetIgnores isEqualTo _ignored)) exitWith {
    [
        KH_var_mouseTargetSurfacePosition,
        KH_var_mouseTargetSurfaceDistance,
        KH_var_mouseTargetSurfaceNormal,
        KH_var_mouseTargetObject,
        KH_var_mouseTargetParentObject,
        KH_var_mouseTargetSelections,
        KH_var_mouseTargetSurfaceType,
        KH_var_mouseTargetDistance
    ];
};

KH_var_mouseTargetIgnores = +_ignored;
KH_var_mouseTargetCheckFrame = diag_frameNo;
private _mousePosition = AGLToASL (positionCameraToWorld [0, 0, 0]);

private _mouseTarget = ([
    _mousePosition, 
    _mousePosition vectorAdd ((screenToWorldDirection getMousePosition) vectorMultiply viewDistance), 
    _ignored,
    true, 
    1, 
    "VIEW", 
    "FIRE", 
    true,
    []
] call KH_fnc_raycast) param [0, []];

KH_var_mouseTargetSurfacePosition = _mouseTarget param [0, _mousePosition];
KH_var_mouseTargetSurfaceDistance = _mousePosition vectorDistance KH_var_mouseTargetSurfacePosition;
KH_var_mouseTargetSurfaceNormal = _mouseTarget param [1, []];
KH_var_mouseTargetObject = _mouseTarget param [2, objNull];
KH_var_mouseTargetParentObject = _mouseTarget param [3, objNull];
KH_var_mouseTargetSelections = _mouseTarget param [4, []];
KH_var_mouseTargetSurfaceType = _mouseTarget param [5, ""];

KH_var_mouseTargetDistance = if !(isNull KH_var_mouseTargetObject) then {
    (getPosATL KH_var_playerUnit) vectorDistance (getPosATL KH_var_mouseTargetObject);
}
else {
    _mousePosition vectorDistance KH_var_mouseTargetSurfacePosition;
};

[
    KH_var_mouseTargetSurfacePosition,
    KH_var_mouseTargetSurfaceDistance,
    KH_var_mouseTargetSurfaceNormal,
    KH_var_mouseTargetObject,
    KH_var_mouseTargetParentObject,
    KH_var_mouseTargetSelections,
    KH_var_mouseTargetSurfaceType,
    KH_var_mouseTargetDistance
];