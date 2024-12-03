params ["_units", ["_steerable", true], ["_equip", false]];

if _equip then {
	private _parachutes = [];

	{
		private _parachute = objNull;
		
		if _steerable then {
			_parachute = createVehicle ["Steerable_Parachute_F", _x, [], 0, "CAN_COLLIDE"];
		}
		else {
			_parachute = createVehicle ["NonSteerable_Parachute_F", _x, [], 0, "CAN_COLLIDE"];
		};

		_parachute setVectorDir (vectorDir _x);
		_parachute setVelocity (velocity _x);
		_x moveInAny _parachute;
		_parachutes pushBack _parachute;
	} forEach _units;

	_parachutes;
}
else {
	{
		private _backpackPresent = false;
		private _backpack = "";
		private _backpackItems = [];

		if !(isNull (unitBackpack _unit)) then {
			_backpackPresent = true;
			_backpack = backpack _unit;
			_backpackItems = backpackItems _unit;
		};

		[
			[_steerable, _x, _backpackPresent, _backpack, _backpackItems],
			{
				params ["_steerable", "_unit", "_backpackPresent", "_backpack", "_backpackItems"];		
				removeBackpack _unit;
				
				if _steerable then {
					_unit addBackpack "B_Parachute";
				}
				else {
					if (uiNamespace getVariable ["KH_var_aceLoaded", false]) then {
						_unit addBackpack "ACE_NonSteerableParachute";
					}
					else {
						_unit addBackpack "B_Parachute";
					};
				};

				if _backpackPresent then {
					_unit addEventHandler [
						"SlotItemChanged", 
						{
							private _unit = _this select 0;
							private _slot = _this select 2;
							private _assigned = _this select 3;

							if ((_slot == 901) && !_assigned) then {
								_unit addBackpack _backpack;

								{
									_unit addItemToBackpack _x;
								} forEach _backpackItems;

								_unit removeEventHandler [_thisEvent, _thisEventHandler];
							};
						}
					];
				};
			},
			_x,
			"THIS_FRAME"
		] call KH_fnc_execute;
	} forEach _units;

	true;
};