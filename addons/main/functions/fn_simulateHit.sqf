params [["_entity", objNull, [objNull]], ["_selection", "", [""]], ["_ammo", "", [""]], ["_parent", objNull, [[], objNull]], ["_terminateOnHit", true, [true]]];

if (_parent isEqualType objNull) then {
    _parent = [vehicle _parent, _parent];
};

private _projectile = createVehicle [
    _ammo, 
    if (_selection isEqualTo "") then {
        ASLToATL (AGLToASL (unitAimPositionVisual _entity));
    }
    else {
        private _position = _entity selectionPosition [_selection, "FireGeometry", "AveragePoint"];
        _position set [2, (_position select 2) max 0.01];
        ASLToATL (_entity modelToWorldVisualWorld _position);
    },
    [], 
    0, 
    "CAN_COLLIDE"
];

_projectile setShotParents _parent;
_projectile setVectorUp ((AGLToASL (_projectile modelToWorldVisual [0, 0, 0])) vectorFromTo (AGLToASL (unitAimPositionVisual _entity)));
_projectile setVelocityModelSpace [0, 0, (getNumber (configFile >> "CfgAmmo" >> _ammo >> "typicalSpeed")) max 1];

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