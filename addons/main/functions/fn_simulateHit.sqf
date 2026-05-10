params [["_entity", objNull, [objNull]], ["_selection", "", [""]], ["_ammo", "", [""]], ["_parent", objNull, [[], objNull]], ["_velocity", [], [[]]], ["_terminateOnHit", true, [true]]];
private _lod = (allLODs _entity) select {(_x select 1) isEqualTo "geometryFire";};

if (_parent isEqualType objNull) then {
    _parent = [vehicle _parent, _parent];
};

if (_lod isEqualTo []) exitWith {
    objNull;
};

private _projectile = createVehicle [
    _ammo, 
    if (_selection isEqualTo "") then {
        ASLToATL (AGLToASL (unitAimPositionVisual _entity));
    }
    else {
        ASLToATL (_entity modelToWorldVisualWorld (selectionPosition [_entity, _selection, (_lod select 0) select 0, false, "AveragePoint"]));
    },
    [], 
    0, 
    "CAN_COLLIDE"
];

_projectile setShotParents _parent;

if (_velocity isEqualTo []) then {
    _projectile setVelocity [0, 0, -(getNumber (configFile >> "CfgAmmo" >> _ammo >> "typicalSpeed"))];
}
else {
    _velocity params [["_projectileVelocity", [0, 0, 0], [[]]], ["_useProjectileMagnitude", true, [true]]];

    if _useProjectileMagnitude then {
        _projectile setVelocity (_projectileVelocity vectorMultiply ((getNumber (configFile >> "CfgAmmo" >> _ammo >> "typicalSpeed")) max 1));
    }
    else {
        _projectile setVelocity _projectileVelocity;
    };
};

if _terminateOnHit then {
    [
        ["ENTITY", _projectile, "LOCAL"],
        "HitPart",
        [],
        {
            params ["_projectile"];
            deleteVehicle _projectile;
        }
    ] call KH_fnc_addEventHandler;
};

_projectile;