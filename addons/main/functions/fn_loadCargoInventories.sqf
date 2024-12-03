params ["_cargo"];

if (_cargo isEqualType []) then {
	KH_var_savedCargoMap = createHashMapFromArray _cargo;
}
else {
	KH_var_savedCargoMap = createHashMapFromArray (missionProfileNamespace getVariable [format ["KH_var_savedCargoInventories_%1", _cargo], []]);
};

{
	private _variableName = vehicleVarName _x;
	private _inventory = KH_var_savedCargoMap get _variableName;
	
	if !(isNil "_inventory") then {
		private _itemInventory = _inventory select 0;
		private _weaponInventory = _inventory select 1;
		private _backpackInventory = _inventory select 2;
		private _aceInventory = _inventory select 3;
		private _holder = _x;
		clearItemCargoGlobal _holder;
		clearWeaponCargoGlobal _holder;
		clearBackpackCargoGlobal _holder;

		if (uiNamespace getVariable ["KH_var_aceLoaded", false]) then {
			private _currentAceCargo = _holder getVariable ["ace_cargo_loaded", []];
			
			{
				[_x, _holder] call ace_cargo_fnc_removeCargoItem;
			} forEach _currentAceCargo;
		};
		
		private _inventorySorted = [];
		private _inventoryItems = _itemInventory select 0;
		private _inventoryCount = (count _inventoryItems) - 1;
		private _inventoryItemCount = _itemInventory select 1;
		private _i = 0;

		for "_i" from 0 to _inventoryCount do {			
			private _sortedItem = [];
			private _item = _inventoryItems select _i;
			private _count = _inventoryItemCount select _i;
			_sortedItem pushBack _item;
			_sortedItem pushBack _count;
			_inventorySorted pushBack _sortedItem;
		};
		
		{		
			_holder addItemCargoGlobal _x;
		} forEach _inventorySorted;
		
		{
			_holder addWeaponWithAttachmentsCargoGlobal [_x, 1];
		} forEach _weaponInventory;
		
		{
			_holder addBackpackCargoGlobal [_x, 1]
		} forEach _backpackInventory;
		
		if (uiNamespace getVariable ["KH_var_aceLoaded", false]) then {
			private _aceCargoCount = 0;
			
			{
				private _currentAceCargo = createVehicle [_x select 0, [0, 0, 0], [], 0, "CAN_COLLIDE"];
				[_currentAceCargo, _holder, true] call ace_cargo_fnc_loadItem;
				_currentAceCargo = _holder getVariable ["ace_cargo_loaded", []];
				_currentAceCargo = _currentAceCargo select _aceCargoCount;
				_aceCargoCount = _aceCargoCount + 1;
				private _itemInventory = (_x select 1) select 0;
				private _weaponInventory = (_x select 1) select 1;
				private _backpackInventory = (_x select 1) select 2;
				private _holder = _currentAceCargo;
				clearItemCargoGlobal _holder;
				clearWeaponCargoGlobal _holder;
				clearBackpackCargoGlobal _holder;
				private _inventorySorted = [];
				private _inventoryItems = _itemInventory select 0;
				private _inventoryCount = (count _inventoryItems) - 1;
				private _inventoryItemCount = _itemInventory select 1;
				private _i = 0;

				for "_i" from 0 to _inventoryCount do {			
					private _sortedItem = [];
					private _item = _inventoryItems select _i;
					private _count = _inventoryItemCount select _i;
					_sortedItem pushBack _item;
					_sortedItem pushBack _count;
					_inventorySorted pushBack _sortedItem;
				};
				
				{		
					_holder addItemCargoGlobal _x;
				} forEach _inventorySorted;
				
				{
					_holder addWeaponWithAttachmentsCargoGlobal [_x, 1];
				} forEach _weaponInventory;
				
				{
					_holder addBackpackCargoGlobal [_x, 1]
				} forEach _backpackInventory;
			} forEach _aceInventory;
		};
	};
} forEach KH_var_allEntities;

true;