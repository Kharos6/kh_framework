params ["_objects", "_position", "_radius", "_damage", "_chance", ["_randomDamage", false], ["_effects", true], ["_dynamicSimulation", false], ["_disableDamage", false], ["_convertSimple", false], ["_initialization", {}]];
private _allObjects = nearestTerrainObjects [_position, _objects, _radius, false, true];
private _currentMissionObjects = entities [[], [], true, false];
private _editedObjects = [missionNamespace, "KH_var_editedTerrainObjects", [], false] call KH_fnc_atomicVariable;

private _processObjects = {
	params ["_object", "_damage", "_dynamicSimulation", "_disableDamage", "_convertSimple", "_initialization", "_currentMissionObjects", "_editedObjects", "_position"];

	[
		{
			params ["_object", "_damage", "_dynamicSimulation", "_disableDamage", "_convertSimple", "_initialization", "_currentMissionObjects", "_editedObjects", "_position"];

			[
				{
					params ["_object", "_damage"];
					((damage _object) == _damage);
				},
				{
					private _dynamicSimulation = _this select 2;
					private _disableDamage = _this select 3;
					private _convertSimple = _this select 4;
					private _initialization = _this select 5;
					private _currentMissionObjects = _this select 6;
					private _editedObjects = _this select 7;
					private _position = _this select 8;
					private _currentEditedObjects = missionNamespace getVariable [_editedObjects, []];

					{
						if !(_x in _currentEditedObjects) then {
							private _dynamicSimulation = _this select 2;
							private _disableDamage = _this select 3;
							private _convertSimple = _this select 4;
							private _initialization = _this select 5;
							private _currentMissionObjects = _this select 6;
							private _position = _this select 8;
							
							if _dynamicSimulation then {	
								[
									[_x],
									{
										params ["_object"];
										_object enableDynamicSimulation true;
									},
									["JIP", "GLOBAL", _x, false, false],
									"THIS_FRAME"
								] call KH_fnc_execute;					
							};
							
							if _disableDamage then {
								[
									[_x],
									{
										params ["_object"];
										_object allowDamage false;
									},
									_x,
									"THIS_FRAME"
								] call KH_fnc_execute;
							};
							
							if _convertSimple then {
								[[_x], true, {}] call KH_fnc_convertToSimpleObject;
							};

							[_x] call _initialization;
							_currentEditedObjects pushBackUnique _x;
							missionNamespace setVariable [_editedObjects, _currentEditedObjects];
						};
					} forEach ((nearestObjects [_position, [], 30, true]) - _currentMissionObjects);			
				},				
				[_object, _damage, _dynamicSimulation, _disableDamage, _convertSimple, _initialization, _currentMissionObjects, _editedObjects, _position], 
				30
			] call CBA_fnc_waitUntilAndExecute;
		}, 
		[_object, _damage, _dynamicSimulation, _disableDamage, _convertSimple, _initialization, _currentMissionObjects, _editedObjects, _position]
	] call CBA_fnc_execNextFrame;
};

if (_damage != 0) then {
	if (_chance == 1) then {
		{
			private _ruinPosition = _x modelToWorld [0, 0, 0];
			
			if !_randomDamage then {
				_x setDamage [_damage, _effects];
				[_x, _damage, _dynamicSimulation, _disableDamage, _convertSimple, _initialization, _currentMissionObjects, _editedObjects, _ruinPosition] call _processObjects;
			}
			else {
				private _damageValue = random _damage;
				_x setDamage [_damageValue, _effects];
				[_x, _damageValue, _dynamicSimulation, _disableDamage, _convertSimple, _initialization, _currentMissionObjects, _editedObjects, _ruinPosition] call _processObjects;
			};
		} forEach _allObjects;
	}
	else {
		{
			private _randomNumber = random 1;
			
			if (_randomNumber <= _chance) then {
				private _ruinPosition = _x modelToWorld [0, 0, 0];
				
				if !_randomDamage then {
					_x setDamage [_damage, _effects];
					[_x, _damage, _dynamicSimulation, _disableDamage, _convertSimple, _initialization, _currentMissionObjects, _editedObjects, _ruinPosition] call _processObjects;
				}
				else {
					private _damageValue = random _damage;
					_x setDamage [_damageValue, _effects];
					[_x, _damageValue, _dynamicSimulation, _disableDamage, _convertSimple, _initialization, _currentMissionObjects, _editedObjects, _ruinPosition] call _processObjects;
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

		[_x] call _initialization;
	} forEach _allObjects;
};

true;