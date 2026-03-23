params [["_entity", objNull, [objNull]], ["_selection", "", [""]], ["_ammo", "", [""]], ["_parent", objNull, [[], objNull]], ["_velocity", [], [[]]]];
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
        ASLToATL (AGLToASL (unitAimPosition _entity));
    }
    else {
        ASLToATL (_entity modelToWorldWorld (selectionPosition [_entity, _selection, (_lod select 0) select 0, false, "AveragePoint"]));
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
    _projectile setVelocity _velocity;
};

_projectile;