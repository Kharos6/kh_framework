params [["_object", objNull, [objNull]], ["_inventory", [], [[]]]];
_inventory params ["_itemInventory", "_weaponInventory", "_backpackInventory", "_aceInventory"];
clearItemCargoGlobal _object;
clearWeaponCargoGlobal _object;
clearBackpackCargoGlobal _object;
private _inventorySorted = [];
_itemInventory params ["_inventoryItems", "_inventoryItemCount"];

for "_i" from 0 to ((count _inventoryItems) - 1) do {
	_inventorySorted pushBack [_inventoryItems select _i, _inventoryItemCount select _i];
};

{		
	_object addItemCargoGlobal _x;
} forEach _inventorySorted;

{
	_object addWeaponWithAttachmentsCargoGlobal [_x, 1];
} forEach _weaponInventory;

{
	_object addBackpackCargoGlobal [_x, 1]
} forEach _backpackInventory;

if KH_var_aceLoaded then {	
	{
		[_x, _object] call ace_cargo_fnc_removeCargoItem;
	} forEach (_object getVariable ["ace_cargo_loaded", []]);

	private _aceCargoCount = 0;
	
	{
		private _currentAceCargo = createVehicle [_x select 0, [0, 0, 0], [], 0, "CAN_COLLIDE"];
		[_currentAceCargo, _object, true] call ace_cargo_fnc_loadItem;
		_currentAceCargo = _object getVariable ["ace_cargo_loaded", []];
		_currentAceCargo = _currentAceCargo select _aceCargoCount;
		_aceCargoCount = _aceCargoCount + 1;
		(_x select 1) params ["_itemInventory", "_weaponInventory", "_backpackInventory"];
		private _object = _currentAceCargo;
		clearItemCargoGlobal _object;
		clearWeaponCargoGlobal _object;
		clearBackpackCargoGlobal _object;
		private _inventorySorted = [];
		_itemInventory params ["_inventoryItems", "_inventoryItemCount"];

		for "_i" from 0 to ((count _inventoryItems) - 1) do {			
			_inventorySorted pushBack [_inventoryItems select _i, _inventoryItemCount select _i];
		};
		
		{		
			_object addItemCargoGlobal _x;
		} forEach _inventorySorted;
		
		{
			_object addWeaponWithAttachmentsCargoGlobal [_x, 1];
		} forEach _weaponInventory;
		
		{
			_object addBackpackCargoGlobal [_x, 1]
		} forEach _backpackInventory;
	} forEach _aceInventory;
};

nil;