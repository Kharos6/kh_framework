params ["_identifier", ["_objects", (entities [[], ["Man", "Logic", "Animal"], false, false])]];

private _cargoEntries = [];

{
	private _variableName = vehicleVarName _x;
	
	if (_variableName != "") then {
		private _className = typeOf _x;
		private _transportBackpack = getNumber (configFile >> "CfgVehicles" >> _className >> "transportmaxbackpacks");
		private _transportMagazines = getNumber (configFile >> "CfgVehicles" >> _className >> "transportmaxmagazines");
		private _transportWeapons = getNumber (configFile >> "CfgVehicles" >> _className >> "transportmaxweapons");
		
		if (((_transportBackpack > 0) || (_transportMagazines > 0) || (_transportWeapons > 0)) || ((_x getVariable ["ace_cargo_loaded", []]) isNotEqualTo [])) then {
			private _cargo = getItemCargo _x;
			private _cargoItems = _cargo select 0;
			private _cargoItemCount = _cargo select 1;
			private _cargoMags = getMagazineCargo _x;
			private _magItems = _cargoMags select 0;
			private _magItemCount = _cargoMags select 1;
			private _cargoWeapons = weaponsItemsCargo _x;
			private _cargoBackpacks = backpackCargo _x;
			private _cargoContainers = everyContainer _x;
			private _aceCargo = _x getVariable ["ace_cargo_loaded", []];
			private _aceCargoItems = [];
			
			{
				_cargoItems pushBack _x;
			} forEach _magItems;
			
			{
				_cargoItemCount pushBack _x;
			} forEach _magItemCount;
			
			{
				private _cargoContainerCargo = getItemCargo (_x select 1);
				private _cargoContainerItems = _cargoContainerCargo select 0;
				private _cargoContainerItemCount = _cargoContainerCargo select 1;
				private _cargoContainerMags = getMagazineCargo (_x select 1);
				private _cargoMagItems = _cargoContainerMags select 0;
				private _cargoMagItemCount = _cargoContainerMags select 1;
				private _cargoContainerWeapons = weaponsItemsCargo (_x select 1);
				_cargoItems append _cargoContainerItems;
				_cargoItemCount append _cargoContainerItemCount;
				_cargoItems append _cargoMagItems;
				_cargoItemCount append _cargoMagItemCount;
				_cargoWeapons append _cargoContainerWeapons;
			} forEach _cargoContainers;
			
			{
				if (_x isEqualType objNull) then {
					private _cargo = getItemCargo _x;
					private _cargoItems = _cargo select 0;
					private _cargoItemCount = _cargo select 1;
					private _cargoMags = getMagazineCargo _x;
					private _magItems = _cargoMags select 0;
					private _magItemCount = _cargoMags select 1;
					private _cargoWeapons = weaponsItemsCargo _x;
					private _cargoBackpacks = backpackCargo _x;
					private _cargoContainers = everyContainer _x;
					
					{
						_cargoItems pushBack _x;
					} forEach _magItems;
					
					{
						_cargoItemCount pushBack _x;
					} forEach _magItemCount;
					
					{
						private _cargoContainerCargo = getItemCargo (_x select 1);
						private _cargoContainerItems = _cargoContainerCargo select 0;
						private _cargoContainerItemCount = _cargoContainerCargo select 1;
						private _cargoContainerMags = getMagazineCargo (_x select 1);
						private _cargoMagItems = _cargoContainerMags select 0;
						private _cargoMagItemCount = _cargoContainerMags select 1;
						private _cargoContainerWeapons = weaponsItemsCargo (_x select 1);
						_cargoItems append _cargoContainerItems;
						_cargoItemCount append _cargoContainerItemCount;
						_cargoItems append _cargoMagItems;
						_cargoItemCount append _cargoMagItemCount;
						_cargoWeapons append _cargoContainerWeapons;
					} forEach _cargoContainers;
					
					_cargo = [[_cargoItems, _cargoItemCount], _cargoWeapons, _cargoBackpacks];
					private _aceCargoFormatted = [typeOf _x, _cargo];
					_aceCargoItems pushBack _aceCargoFormatted;
				}
				else {
					private _aceCargoFormatted = [_x, [[[], []], [], []]];
					_aceCargoItems pushBack _aceCargoFormatted;
				};
			} forEach _aceCargo;
			
			_cargoEntries pushBack [_variableName, [[_cargoItems, _cargoItemCount], _cargoWeapons, _cargoBackpacks, _aceCargoItems]];
		};
	};
} forEach _objects;

[
	[_identifier, _cargoEntries], 
	{
		params ["_identifier", "_cargoEntries"];
		[format ["KH_var_savedCargoInventories_%1", _identifier], _cargoEntries] call KH_fnc_editMissionProfileNamespace;
	},
	"GLOBAL",
	"THIS_FRAME"
] call KH_fnc_execute;

_cargoEntries;