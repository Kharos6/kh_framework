params ["_units", ["_steerable", true], ["_equip", false]];

if _equip then {
	private _parachutes = [];

	{
		if (isNull (objectParent _x)) then {
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
		};
	} forEach _units;

	_parachutes;
}
else {
	{
		private _backpackPresent = false;
		private _backpack = "";
		private _backpackItems = [];

		if !(isNull (unitBackpack _x)) then {
			_backpackPresent = true;
			_backpack = backpack _x;
			_backpackItems = backpackItems _x;
		};

		[
			[_steerable, _x, _backpackPresent, _backpack, _backpackItems],
			{
				params ["_steerable", "_unit", "_backpackPresent", "_backpack", "_backpackItems"];

				if (((backpack _unit) != "B_Parachute") && ((backpack _unit) != "ACE_NonSteerableParachute")) then {
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
						[
							{
								params ["_unit"];
								(((backpack _unit) == "B_Parachute") || ((backpack _unit) == "ACE_NonSteerableParachute"));
							}, 
							{
								params ["_unit", "_backpack", "_backpackItems"];

								[
									{
										params ["_unit", "_backpack", "_backpackItems"];

										[
											["STANDARD", _unit, false],
											"SlotItemChanged",
											[_backpack, _backpackItems],
											{
												_args params ["_backpack", "_backpackItems"];
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
										] call KH_fnc_addEventHandler;
									}, 
									[_unit, _backpack, _backpackItems]
								] call CBA_fnc_execNextFrame;
							}, 
							[_unit, _backpack, _backpackItems],
							30
						] call CBA_fnc_waitUntilAndExecute;
					};
				};
			},
			_x,
			"THIS_FRAME"
		] call KH_fnc_execute;
	} forEach _units;

	true;
};