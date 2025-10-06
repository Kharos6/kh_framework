params [["_object", objNull, [objNull]], ["_loadout", [], [[]]]];
private _componentsId = generateUid;

[
    fuel _object,
    getAmmoCargo _object,
    getFuelCargo _object,
    getRepairCargo _object,
    _object weaponsInfo ["", false],
    getAllPylonsInfo _object,
    call {
        private _pylonAmmo = [];

        {
            _pylonAmmo pushBack [_x select 0, _x select 4];
        } forEach (getAllPylonsInfo _object);

        _pylonAmmo;
    },
    getPlateNumber _object,
    [
        _componentsId,
        [_object, [_object, _componentsId], [], false] call BIS_fnc_saveVehicle
    ]
];