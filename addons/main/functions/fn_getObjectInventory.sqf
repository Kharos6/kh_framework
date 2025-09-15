params [["_object", objNull, [objNull]]];
private _className = typeOf _object;
private _transportBackpack = getNumber (configFile >> "CfgVehicles" >> _className >> "transportmaxbackpacks");
private _transportMagazines = getNumber (configFile >> "CfgVehicles" >> _className >> "transportmaxmagazines");
private _transportWeapons = getNumber (configFile >> "CfgVehicles" >> _className >> "transportmaxweapons");

if (((_transportBackpack isEqualTo 0) || (_transportMagazines isEqualTo 0) || (_transportWeapons isEqualTo 0)) && ((_object getVariable ["ace_cargo_loaded", []]) isEqualTo [])) exitWith {
	[];
};

(getItemCargo _object) params ["_cargoItems", "_cargoItemCount"];
private _cargoMags = getMagazineCargo _object;
private _cargoWeapons = weaponsItemsCargo _object;
private _cargoContainers = everyContainer _object;
private _aceCargo = _object getVariable ["ace_cargo_loaded", []];
private _aceCargoItems = [];
_cargoItems append (_cargoMags select 0);
_cargoItemCount append (_cargoMags select 1);

{
	private _cargoContainerCargo = getItemCargo (_x select 1);
	private _cargoContainerMags = getMagazineCargo (_x select 1);
	_cargoItems append (_cargoContainerCargo select 0);
	_cargoItemCount append (_cargoContainerCargo select 1);
	_cargoItems append (_cargoContainerMags select 0);
	_cargoItemCount append (_cargoContainerMags select 1);
	_cargoWeapons append (weaponsItemsCargo (_x select 1));
} forEach _cargoContainers;

{
	if (_x isEqualType objNull) then {
		(getItemCargo _x) params ["_cargoItems", "_cargoItemCount"];
		private _cargoMags = getMagazineCargo _x;
		private _cargoWeapons = weaponsItemsCargo _x;
		private _cargoContainers = everyContainer _x;
		_cargoItems append (_cargoMags select 0);
		_cargoItemCount append (_cargoMags select 1);
		
		{
			private _cargoContainerCargo = getItemCargo (_x select 1);
			private _cargoContainerMags = getMagazineCargo (_x select 1);
			_cargoItems append (_cargoContainerCargo select 0);
			_cargoItemCount append (_cargoContainerCargo select 1);
			_cargoItems append (_cargoContainerMags select 0);
			_cargoItemCount append (_cargoContainerMags select 1);
			_cargoWeapons append (weaponsItemsCargo (_x select 1));
		} forEach _cargoContainers;
		
		_aceCargoItems pushBack [typeOf _x, [[_cargoItems, _cargoItemCount], _cargoWeapons, backpackCargo _x]];
	}
	else {
		_aceCargoItems pushBack [_x, [[[], []], [], []]];
	};
} forEach _aceCargo;

[[_cargoItems, _cargoItemCount], _cargoWeapons, backpackCargo _object, _aceCargoItems];