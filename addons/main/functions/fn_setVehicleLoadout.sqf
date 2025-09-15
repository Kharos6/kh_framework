params [["_object", objNull, [objNull]], ["_loadout", [], [[]]]];

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

_object setFuel _fuel;
_object setAmmoCargo _ammoCargo;
_object setFuelCargo _fuelCargo;
_object setRepairCargo _repairCargo;

{
    _object setAmmo [_x select 3, _x select 6];
} forEach _weaponsInfo;

{
    _object setPylonLoadout [_x select 0, _x select 3, true];
} forEach _pylonsInfo;

{
    _object setAmmoOnPylon _x;
} forEach _pylonsAmmo;

_object setPlateNumber _plateNumber;
_components params ["_componentsId", "_allComponents"];
private _currentVehicleData = _object getVariable ["bis_fnc_saveVehicle_data", []];

if (_currentVehicleData isEqualTo []) then {
    _object setVariable ["bis_fnc_saveVehicle_data", [_componentsId, _allComponents]];
}
else {
    if (_componentsId in _currentVehicleData) then {
        private _index = _currentVehicleData find _componentsId;
        _currentVehicleData deleteAt [_index, _index + 1];
    };

    _currentVehicleData pushBack _componentsId;
    _currentVehicleData pushBack _allComponents;
};

[_object, [_object, _componentsId]] call BIS_fnc_loadVehicle;