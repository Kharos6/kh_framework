params [["_vehicle", objNull, [objNull]], ["_loadout", [], [[]]]];
private _empty = (crew _vehicle) isEqualTo [];

_loadout params [
    "_fuel",
    "_ammoCargo",
    "_fuelCargo",
    "_repairCargo",
    "_weaponsInfo",
    "_pylonsInfo",
    "_pylonsAmmo",
    "_plateNumber",
    "_components"
];

_vehicle setFuel _fuel;
_vehicle setAmmoCargo _ammoCargo;
_vehicle setFuelCargo _fuelCargo;
_vehicle setRepairCargo _repairCargo;

{
    _vehicle setAmmo [_x select 3, _x select 6];
} forEach _weaponsInfo;

{
    _vehicle setPylonLoadout [_x select 0, _x select 3, true, _x select 2];
} forEach _pylonsInfo;

{
    _vehicle setAmmoOnPylon _x;
} forEach _pylonsAmmo;

_vehicle setPlateNumber _plateNumber;
_components params ["_componentsId", "_allComponents"];
_vehicle setVariable ["bis_fnc_saveVehicle_data", [_componentsId, _allComponents]];
[_vehicle, [_vehicle, _componentsId]] call BIS_fnc_loadVehicle;

if _empty then {
    deleteVehicleCrew _vehicle;    
};

nil;