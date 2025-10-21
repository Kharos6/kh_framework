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

if ((KH_var_weaponTargetCheckFrame isEqualTo diag_frameNo) && (KH_var_weaponTargetIgnores isEqualTo _ignored)) exitWith {
    [
        KH_var_weaponTargetSurfacePosition,
        KH_var_weaponTargetSurfaceDistance,
        KH_var_weaponTargetSurfaceNormal,
        KH_var_weaponTargetObject,
        KH_var_weaponTargetParentObject,
        KH_var_weaponTargetSelections,
        KH_var_weaponTargetSurfaceType,
        KH_var_weaponTargetDistance
    ];
};

KH_var_weaponTargetIgnores = +_ignored;
KH_var_weaponTargetCheckFrame = diag_frameNo;
private _currentWeapon = currentWeapon KH_var_playerUnit;

private _weaponType = switch _currentWeapon do {
    case (primaryWeapon KH_var_playerUnit): {
        "PRIMARY";
    };

    case (handgunWeapon KH_var_playerUnit): {
        "SECONDARY";
    };

    case (secondaryWeapon KH_var_playerUnit): {
        "TERTIARY";
    };

    default {
        "";
    };
};

private _weaponPosition = if (_weaponType isEqualTo "") then {
    AGLToASL (positionCameraToWorld [0, 0, 0]);
}
else {
    AGLToASL ([KH_var_playerUnit, _weaponType] call KH_fnc_getWeaponPosition);
};

private _weaponTarget = ([
    _weaponPosition,
    if (_currentWeapon isNotEqualTo "") then {
        _weaponPosition vectorAdd ((KH_var_playerUnit weaponDirection _currentWeapon) vectorMultiply viewDistance);
    }
    else {
        _weaponPosition vectorAdd ((getCameraViewDirection KH_var_playerUnit) vectorMultiply viewDistance);
    },
    _ignored,
    true, 
    1, 
    "VIEW", 
    "FIRE", 
    true,
    []
] call KH_fnc_raycast) param [0, []];

KH_var_weaponTargetSurfacePosition = _weaponTarget param [0, _weaponPosition];
KH_var_weaponTargetSurfaceDistance = _weaponPosition vectorDistance KH_var_weaponTargetSurfacePosition;
KH_var_weaponTargetSurfaceNormal = _weaponTarget param [1, []];
KH_var_weaponTargetObject = _weaponTarget param [2, objNull];
KH_var_weaponTargetParentObject = _weaponTarget param [3, objNull];
KH_var_weaponTargetSelections = _weaponTarget param [4, []];
KH_var_weaponTargetSurfaceType = _weaponTarget param [5, ""];

KH_var_weaponTargetDistance = if !(isNull KH_var_weaponTargetObject) then {
    (getPosATL KH_var_playerUnit) vectorDistance (getPosATL KH_var_weaponTargetObject);
}
else {
    _weaponPosition vectorDistance KH_var_weaponTargetSurfacePosition;
};

[
    KH_var_weaponTargetSurfacePosition,
    KH_var_weaponTargetSurfaceDistance,
    KH_var_weaponTargetSurfaceNormal,
    KH_var_weaponTargetObject,
    KH_var_weaponTargetParentObject,
    KH_var_weaponTargetSelections,
    KH_var_weaponTargetSurfaceType,
    KH_var_weaponTargetDistance
];