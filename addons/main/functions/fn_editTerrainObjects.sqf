params ["_objects", "_position", "_radius", "_damage", "_chance", ["_randomDamage", false], ["_effects", true], ["_dynamicSimulation", false], ["_disableDamage", false], ["_convertSimple", false]];
private _allObjects = nearestTerrainObjects [_position, _objects, _radius, false, true];

if (_damage != 0) then {
	if (_chance == 1) then {
		{
			private _ruinPosition = [_x, "AGL", []] call KH_fnc_getPosition;
			
			if !_randomDamage then {
				_x setDamage [_damage, _effects];
				
				[
					{
						params ["_object", "_damage"];
						((damage _object) == _damage);
					},
					{
						params ["_object", "_damage", "_dynamicSimulation", "_disableDamage", "_convertSimple", "_allObjects", "_ruinPosition"];

						if _dynamicSimulation then {	
							[
								[_object],
								{
									params ["_object"];
									_object enableDynamicSimulation true;
								},
								["JIP", "GLOBAL", _object, false, false],
								"THIS_FRAME"
							] call KH_fnc_execute;					
						};
						
						if _disableDamage then {
							[
								[_object],
								{
									params ["_object"];
									_object allowDamage false;
								},
								_object,
								"THIS_FRAME"
							] call KH_fnc_execute;
						};
						
						if _convertSimple then {
							[[_x], true, {}] call KH_fnc_convertToSimpleObject;
						};			
					},				
					[_x, _damage, _dynamicSimulation, _disableDamage, _convertSimple, _allObjects, _ruinPosition], 
					30
				] call CBA_fnc_waitUntilAndExecute;
			}
			else {
				private _damageValue = random _damage;
				_x setDamage [_damageValue, _effects];
				
				[
					{
						params ["_object", "_damageValue"];	
						((damage _object) == _damageValue);
					},
					{
						params ["_object", "_damageValue", "_dynamicSimulation", "_disableDamage", "_convertSimple"];
					
						if _dynamicSimulation then {		
							[
								[_object],
								{
									params ["_object"];
									_object enableDynamicSimulation true;
								},
								["JIP", "GLOBAL", _object, false, false],
								"THIS_FRAME"
							] call KH_fnc_execute;				
						};
						
						if _disableDamage then {
							[
								[_object],
								{
									params ["_object"];
									_object allowDamage false;
								},
								_object,
								"THIS_FRAME"
							] call KH_fnc_execute;
						};
						
						if _convertSimple then {
							[[_x], true, {}] call KH_fnc_convertToSimpleObject;
						};			
					},				
					[_x, _damageValue, _dynamicSimulation, _disableDamage, _convertSimple],
					30
				] call CBA_fnc_waitUntilAndExecute;
			};
		} forEach _sortedObjects;
	}
	else {
		{
			private _randomNumber = random 1;
			
			if (_randomNumber <= _chance) then {
				private _ruinPosition = [_x, "AGL", []] call KH_fnc_getPosition;
				
				if !_randomDamage then {
					_x setDamage [_damage, _effects];
					
					[
						{
							params ["_object", "_damage"];
							((damage _object) == _damage);
						},
						{
							params ["_object", "_damage", "_dynamicSimulation", "_disableDamage", "_convertSimple", "_allObjects", "_ruinPosition"];

							if _dynamicSimulation then {						
								[
									[_object],
									{
										params ["_object"];
										_object enableDynamicSimulation true;
									},
									["JIP", "GLOBAL", _object, false, false],
									"THIS_FRAME"
								] call KH_fnc_execute;	
							};
							
							if _disableDamage then {
								[
									[_object],
									{
										params ["_object"];
										_object allowDamage false;
									},
									_object,
									"THIS_FRAME"
								] call KH_fnc_execute;
							};
							
							if _convertSimple then {
								[[_x], true, {}] call KH_fnc_convertToSimpleObject;
							};			
						},				
						[_x, _damage, _dynamicSimulation, _disableDamage, _convertSimple, _allObjects, _ruinPosition], 
						30
					] call CBA_fnc_waitUntilAndExecute;
				}
				else {
					private _damageValue = random _damage;
					_x setDamage [_damageValue, _effects];
					
					[
						{
							params ["_object", "_damageValue"];
							((damage _object) == _damageValue);
						},
						{
							params ["_object", "_damageValue", "_dynamicSimulation", "_disableDamage", "_convertSimple"];
						
							if _dynamicSimulation then {						
								[
									[_object],
									{
										params ["_object"];
										_object enableDynamicSimulation true;
									},
									["JIP", "GLOBAL", _object, false, false],
									"THIS_FRAME"
								] call KH_fnc_execute;	
							};
							
							if _disableDamage then {
								[
									[_object],
									{
										params ["_object"];
										_object allowDamage false;
									},
									_object,
									"THIS_FRAME"
								] call KH_fnc_execute;
							};
							
							if _convertSimple then {
								[[_x], true, {}] call KH_fnc_convertToSimpleObject;
							};			
						},				
						[_x, _damageValue, _dynamicSimulation, _disableDamage, _convertSimple], 
						30
					] call CBA_fnc_waitUntilAndExecute;
				};
			};
		} forEach _allObjects;
	};
}
else {
	{		
		if _dynamicSimulation then {									
			[
				[_object],
				{
					_object enableDynamicSimulation true;
				},
				["JIP", "GLOBAL", _object, false, false],
				"THIS_FRAME"
			] call KH_fnc_execute;	
		};

		if _disableDamage then {
			[
				[_object],
				{
					_object allowDamage false;
				},
				_object,
				"THIS_FRAME"
			] call KH_fnc_execute;
		};
		
		if _convertSimple then {
			[[_x], true, {}] call KH_fnc_convertToSimpleObject;
		};
	} forEach _allObjects;
};

true;