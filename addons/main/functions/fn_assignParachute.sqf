params [["_unit", objNull, [objNull]], ["_steerable", true, [true]], ["_equip", false, [false]]];

if _equip then {
	private "_parachute";

	if (isNull (objectParent _unit)) then {		
		if _steerable then {
			_parachute = createVehicle ["Steerable_Parachute_F", _unit, [], 0, "CAN_COLLIDE"];
		}
		else {
			_parachute = createVehicle ["NonSteerable_Parachute_F", _unit, [], 0, "CAN_COLLIDE"];
		};

		_parachute setVectorDir (vectorDir _unit);
		_parachute setVelocity (velocity _unit);
		_unit moveInAny _parachute;
	};

	_parachute;
}
else {
	private _backpackPresent = false;
	private _backpack = "";
	private _backpackItems = [];

	if !(isNull (unitBackpack _unit)) then {
		_backpackPresent = true;
		_backpack = backpack _unit;
		_backpackItems = backpackItems _unit;
	};

	[
		[_steerable, _unit, _backpackPresent, _backpack, _backpackItems],
		{
			params ["_steerable", "_unit", "_backpackPresent", "_backpack", "_backpackItems"];

			if (((backpack _unit) isNotEqualTo "B_Parachute") && ((backpack _unit) isNotEqualTo "ACE_NonSteerableParachute")) then {
				removeBackpack _unit;
				
				if _steerable then {
					_unit addBackpack "B_Parachute";
				}
				else {
					if KH_var_aceLoaded then {
						_unit addBackpack "ACE_NonSteerableParachute";
					}
					else {
						_unit addBackpack "B_Parachute";
					};
				};

				if _backpackPresent then {
					[
						[_unit, _backpack, _backpackItems],
						{
							params ["_unit", "_backpack", "_backpackItems"];
							
							[
								["ENTITY", _unit, "LOCAL"],
								"SlotItemChanged",
								[_backpack, _backpackItems],
								{
									_args params ["_backpack", "_backpackItems"];
									private _unit = param [0];
									private _slot = param [2];
									private _assigned = param [3];

									if ((_slot isEqualTo 901) && !_assigned) then {
										_unit addBackpack _backpack;

										{
											_unit addItemToBackpack _x;
										} forEach _backpackItems;

										[_handlerId] call KH_fnc_removeHandler;
									};
								}
							] call KH_fnc_addEventHandler;
						}, 
						true,
						[
							{
								params ["_unit"];
								(((backpack _unit) isEqualTo "B_Parachute") || ((backpack _unit) isEqualTo "ACE_NonSteerableParachute"));
							},
							true,
							1,
							0,
							true,
							true
						],
						false
					] call KH_fnc_execute;
				};
			};
		},
		_unit,
		true,
		false
	] call KH_fnc_execute;

	nil;
};