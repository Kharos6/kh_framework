params [["_vehicle", objNull, [objNull]], ["_loadout", [], [[]]]];

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
    _vehicle setPylonLoadout [_x select 0, _x select 3, true];
} forEach _pylonsInfo;

{
    _vehicle setAmmoOnPylon _x;
} forEach _pylonsAmmo;

_vehicle setPlateNumber _plateNumber;
_components params ["_componentsId", "_allComponents"];
private _currentVehicleData = _vehicle getVariable ["bis_fnc_saveVehicle_data", []];

if (_currentVehicleData isEqualTo []) then {
    _vehicle setVariable ["bis_fnc_saveVehicle_data", [_componentsId, _allComponents]];
}
else {
    if (_componentsId in _currentVehicleData) then {
        private _index = _currentVehicleData find _componentsId;
        _currentVehicleData deleteAt [_index, _index + 1];
    };

    _currentVehicleData pushBack _componentsId;
    _currentVehicleData pushBack _allComponents;
};

[_vehicle, [_vehicle, _componentsId]] call BIS_fnc_loadVehicle;
nil;