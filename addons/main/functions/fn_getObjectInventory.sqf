params [["_object", objNull, [objNull]]];
(getItemCargo _object) params ["_cargoItems", "_cargoItemCount"];
private _cargoMags = magazinesAmmoCargo _object;
private _cargoWeapons = weaponsItemsCargo _object;
private _cargoContainers = everyContainer _object;
private _aceCargo = _object getVariable ["ace_cargo_loaded", []];
private _aceCargoItems = [];

{
	private _cargoContainerCargo = getItemCargo (_x select 1);
	private _cargoContainerMags = magazinesAmmoCargo (_x select 1);
	_cargoItems append (_cargoContainerCargo select 0);
	_cargoItemCount append (_cargoContainerCargo select 1);
	_cargoMags append _cargoContainerMags;
	_cargoWeapons append (weaponsItemsCargo (_x select 1));
} forEach _cargoContainers;

{
	if (_x isEqualType objNull) then {
		(getItemCargo _x) params ["_cargoItems", "_cargoItemCount"];
		private _cargoMags = magazinesAmmoCargo _x;
		private _cargoWeapons = weaponsItemsCargo _x;
		private _cargoContainers = everyContainer _x;
		
		{
			private _cargoContainerCargo = getItemCargo (_x select 1);
			private _cargoContainerMags = magazinesAmmoCargo (_x select 1);
			_cargoItems append (_cargoContainerCargo select 0);
			_cargoItemCount append (_cargoContainerCargo select 1);
			_cargoMags append _cargoContainerMags;
			_cargoWeapons append (weaponsItemsCargo (_x select 1));
		} forEach _cargoContainers;
		
		_aceCargoItems pushBack [typeOf _x, [[_cargoItems, _cargoItemCount], _cargoMags, _cargoWeapons, backpackCargo _x]];
	}
	else {
		_aceCargoItems pushBack [_x, [[[], []], [], [], []]];
	};
} forEach _aceCargo;

[[_cargoItems, _cargoItemCount], _cargoMags, _cargoWeapons, backpackCargo _object, _aceCargoItems];