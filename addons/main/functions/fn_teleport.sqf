params ["_units", "_position", "_rotation", ["_eject", true], ["_transition", 1], ["_heal", false], ["_freefallHeight", -1], ["_initialization", {}]];

{
	if (_freefallHeight != 0) then {
		[
			[_x, _freefallHeight],
			{
				params ["_unit", "_freefallHeight"];
				_unit setUnitFreefallHeight _freefallHeight;
			},
			_x,
			"THIS_FRAME"
		] call KH_fnc_execute;
	};

	if (_transition != 0) then {
		if (isPlayer _x) then {
			[
				[],
				{
					titleText [" ", "BLACK OUT", 1];
				},
				_x,
				"THIS_FRAME"
			] call KH_fnc_execute;
		};

		[
			{
				params ["_unit", "_heal", "_position", "_rotation", "_eject", "_initialization"];

				if _eject then {
					moveOut _unit;

					[
						{
							params ["_unit"];
							(isNull (objectParent _unit));
						}, 
						{
							params ["_unit", "_position", "_rotation", "_initialization"];

							[
								{
									params ["_unit", "_position", "_rotation", "_initialization"];
									[_unit, [_position, "ATL", false], [_rotation, false]] call KH_fnc_setTransforms;
									[_unit] call _initialization;
								}, 
								[_unit, _position, _rotation, _initialization]
							] call CBA_fnc_execNextFrame;
						}, 
						[_unit, _position, _rotation, _initialization],
						15
					] call CBA_fnc_waitUntilAndExecute;
				}
				else {
					[_unit, [_position, "ATL", false], [_rotation, false]] call KH_fnc_setTransforms;
					[_unit] call _initialization;
				};
				
				if (isPlayer _unit) then {
					[
						[],
						{
							titleText [" ", "BLACK IN", 1];
						},
						_unit,
						"THIS_FRAME"
					] call KH_fnc_execute;
				};

				if _heal then {
					[
						[_unit],
						{
							params ["_unit"];
							
							if (uiNamespace getVariable ["KH_var_aceLoaded", false]) then {
								if ace_medical then {
									[_unit] call ace_medical_treatment_fnc_fullHealLocal;
								}
								else {
									_unit setDamage 0;
								};
							}
							else {
								_unit setDamage 0;
							};
						},
						_unit,
						"THIS_FRAME"
					] call KH_fnc_execute;
				};
			},
			[_x, _heal, _position, _rotation, _eject, _initialization],
			_transition
		] call CBA_fnc_waitAndExecute;
	}
	else {
		if _eject then {
			moveOut _x;
			
			[
				{
					params ["_unit"];
					(isNull (objectParent _unit));
				}, 
				{
					params ["_unit", "_position", "_rotation", "_initialization"];
					
					[
						{
							params ["_unit", "_position", "_rotation", "_initialization"];
							[_unit, [_position, "ATL", false], [_rotation, false]] call KH_fnc_setTransforms;
							[_unit] call _initialization;
						}, 
						[_unit, _position, _rotation]
					] call CBA_fnc_execNextFrame;
				}, 
				[_unit, _position, _rotation, _initialization],
				15
			] call CBA_fnc_waitUntilAndExecute;
		}
		else {
			[_unit, [_position, "ATL", false], [_rotation, false]] call KH_fnc_setTransforms;
			[_unit] call _initialization;
		};
		
		if _heal then {
			[
				[_x],
				{
					params ["_unit"];
					
					if (uiNamespace getVariable ["KH_var_aceLoaded", false]) then {
						if ace_medical then {
							[_unit] call ace_medical_treatment_fnc_fullHealLocal;
						}
						else {
							_unit setDamage 0;
						};
					}
					else {
						_unit setDamage 0;
					};
				},
				_x,
				"THIS_FRAME"
			] call KH_fnc_execute;
		};
	};
} forEach _units;

true;