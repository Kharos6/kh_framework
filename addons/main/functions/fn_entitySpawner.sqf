params [
	["_entityTypes", [], [[]]], 
	["_transforms", [[[0, 0, 0], [0, 0, 0]]], [[]]], 
	["_radius", [100, 100, 0], [[]]], 
	["_amount", 1, [0]], 
	["_maximum", 1, [0]], 
	["_condition", {true;}, [{}]], 
	["_init", {}, [{}]], 
	["_type", [], [[]]],
	["_interval", 1, [0]],
	["_countKilled", true, [true]],
	["_validatePosition", true, [true]],
	["_minimumPlayerDistance", 0, [0]]
];

private _spawnerCount = generateUid;
missionNamespace setVariable [_spawnerCount, 0];

private _entityHandler = [
	"CBA",
	"KH_eve_spawnedEntityTerminated",
	[_spawnerCount],
	{
		_args params ["_spawnerCount"];
		missionNamespace setVariable [_spawnerCount, (missionNamespace getVariable [_spawnerCount, 0]) - 1];
	}
] call KH_fnc_addEventHandler;

private _spawnHandler = [
	[_entityTypes, _transforms, _radius, _amount, _maximum, _condition, _init, _type, _countKilled, _validatePosition, _minimumPlayerDistance, _spawnerCount],
	{
		params ["_entityTypes", "_transforms", "_radius", "_amount", "_maximum", "_condition", "_init", "_type", "_countKilled", "_validatePosition", "_minimumPlayerDistance", "_spawnerCount"];
		private _negativeRadius = [-(_radius select 0), -(_radius select 1), -(_radius select 2)];

		if ((missionNamespace getVariable [_spawnerCount, 0]) < (_maximum + _amount)) then {
			private _spawnedEntities = [];
			private _entityType = _type param [0, "UNIT", [""]];

			switch _entityType do {
				case "UNIT": {
					(_type select [1]) params [["_placementMode", "CAN_COLLIDE", [""]], ["_side", sideUnknown, [sideUnknown]], ["_shareGroup", true, [true]]];
					private _group = grpNull;

					if _shareGroup then {
						_group = createGroup [_side, true];
					};

					for "_i" from 1 to _amount do {
						if !_shareGroup then {
							_group = createGroup [_side, true];
						};

						private _chosenTransforms = selectRandom _transforms;

						private _position = if ((_chosenTransforms select 0) isEqualType objNull) then {
							(_chosenTransforms select 0) modelToWorld [0, 0, 0];
						}
						else {
							_chosenTransforms select 0;
						};

						if !([_chosenTransforms, missionNamespace getVariable [_spawnerCount, 0]] call _condition) then {
							continue;
						};

						private _spawnValid = true;

						if _validatePosition then {
							{
								if ([_x, AGLToASL _position, _x, 1, _minimumPlayerDistance, 0, objNull] call KH_fnc_getPositionVisibility) then {
									_spawnValid = false;
									break;
								};
							} forEach KH_var_allPlayerUnits;
						};

						if !_spawnValid then {
							continue;
						};

						private _rotation = if ((count _chosenTransforms) > 1) then {
							if ((_chosenTransforms select 1) isEqualType objNull) then {
								private _chosenEntity = _chosenTransforms select 1;
								[vectorDir _chosenEntity, vectorUp _chosenEntity];
							}
							else {
								if ((_chosenTransforms select 1) isEqualTypeAll []) then {
									private _vectors = _chosenTransforms select 1;
									[_vectors select 0, _vectors select 1];
								}
								else {
									_chosenTransforms select 1;
								};
							};
						}
						else {
							[[0, 1, 0], [0, 0, 1]];
						};
						
						_position vectorAdd [random [(_radius select 0), 0, (_negativeRadius select 0)], random [(_radius select 1), 0, (_negativeRadius select 1)], random [(_radius select 2), 0, (_negativeRadius select 2)]];

						if !([_position, missionNamespace getVariable [_spawnerCount, 0]] call _condition) then {
							continue;
						};

						private _unit = _group createUnit [selectRandom _entityTypes, _position, [], 0, _placementMode];
						
						if (_rotation isEqualTypeAll []) then {
							_unit setVectorDirAndUp _rotation;
						}
						else {
							_unit setRotationEuler _rotation;
						};

						[_unit, _chosenTransforms, _position] call _init;
						_spawnedEntities pushBack _unit;
					};
				};

				case "AGENT": {
					private _placementMode = _type param [1, "CAN_COLLIDE", [""]];

					for "_i" from 1 to _amount do {
						private _chosenTransforms = selectRandom _transforms;

						private _position = if ((_chosenTransforms select 0) isEqualType objNull) then {
							(_chosenTransforms select 0) modelToWorld [0, 0, 0];
						}
						else {
							_chosenTransforms select 0;
						};

						if !([_chosenTransforms, missionNamespace getVariable [_spawnerCount, 0]] call _condition) then {
							continue;
						};

						private _spawnValid = true;

						if _validatePosition then {
							{
								if ([_x, AGLToASL _position, _x, 1, _minimumPlayerDistance, 0, objNull] call KH_fnc_getPositionVisibility) then {
									_spawnValid = false;
									break;
								};
							} forEach KH_var_allPlayerUnits;
						};

						if !_spawnValid then {
							continue;
						};

						private _rotation = if ((count _chosenTransforms) > 1) then {
							if ((_chosenTransforms select 1) isEqualType objNull) then {
								private _chosenEntity = _chosenTransforms select 1;
								[vectorDir _chosenEntity, vectorUp _chosenEntity];
							}
							else {
								if ((_chosenTransforms select 1) isEqualTypeAll []) then {
									private _vectors = _chosenTransforms select 1;
									[_vectors select 0, _vectors select 1];
								}
								else {
									_chosenTransforms select 1;
								};
							};
						}
						else {
							[[0, 1, 0], [0, 0, 1]];
						};

						_position vectorAdd [random [(_radius select 0), 0, (_negativeRadius select 0)], random [(_radius select 1), 0, (_negativeRadius select 1)], random [(_radius select 2), 0, (_negativeRadius select 2)]];

						if !([_position, missionNamespace getVariable [_spawnerCount, 0]] call _condition) then {
							continue;
						};

						private _agent = createAgent [selectRandom _entityTypes, _position, [], 0, _placementMode];

						if (_rotation isEqualTypeAll []) then {
							_agent setVectorDirAndUp _rotation;
						}
						else {
							_agent setRotationEuler _rotation;
						};

						[_agent, _chosenTransforms, _position] call _init;
						_spawnedEntities pushBack _agent;
					};
				};

				case "OBJECT": {
					(_type select [1]) params [["_placementMode", "CAN_COLLIDE", [""]], ["_local", false, [true]]];

					for "_i" from 1 to _amount do {
						private _chosenTransforms = selectRandom _transforms;

						private _position = if ((_chosenTransforms select 0) isEqualType objNull) then {
							(_chosenTransforms select 0) modelToWorld [0, 0, 0];
						}
						else {
							_chosenTransforms select 0;
						};

						if !([_chosenTransforms, missionNamespace getVariable [_spawnerCount, 0]] call _condition) then {
							continue;
						};

						private _spawnValid = true;

						if _validatePosition then {
							{
								if ([_x, AGLToASL _position, _x, 1, _minimumPlayerDistance, 0, objNull] call KH_fnc_getPositionVisibility) then {
									_spawnValid = false;
									break;
								};
							} forEach KH_var_allPlayerUnits;
						};

						if !_spawnValid then {
							continue;
						};

						private _rotation = if ((count _chosenTransforms) > 1) then {
							if ((_chosenTransforms select 1) isEqualType objNull) then {
								private _chosenEntity = _chosenTransforms select 1;
								[vectorDir _chosenEntity, vectorUp _chosenEntity];
							}
							else {
								if ((_chosenTransforms select 1) isEqualTypeAll []) then {
									private _vectors = _chosenTransforms select 1;
									[_vectors select 0, _vectors select 1];
								}
								else {
									_chosenTransforms select 1;
								};
							};
						}
						else {
							[[0, 1, 0], [0, 0, 1]];
						};

						_position vectorAdd [random [(_radius select 0), 0, (_negativeRadius select 0)], random [(_radius select 1), 0, (_negativeRadius select 1)], random [(_radius select 2), 0, (_negativeRadius select 2)]];

						if !([_position, missionNamespace getVariable [_spawnerCount, 0]] call _condition) then {
							continue;
						};

						private _object = if !_local then {
							_object = createVehicle [selectRandom _entityTypes, _position, [], 0, _placementMode];
						}
						else {
							_object = createVehicleLocal [selectRandom _entityTypes, _position, [], 0, _placementMode];
						};

						if (_rotation isEqualTypeAll []) then {
							_object setVectorDirAndUp _rotation;
						}
						else {
							_object setRotationEuler _rotation;
						};

						[_object, _chosenTransforms, _position] call _init;
						_spawnedEntities pushBack _object;
					};
				};

				case "SIMPLE_OBJECT": {
					private _local = _type param [1, false, [true]];

					for "_i" from 1 to _amount do {
						private _chosenTransforms = selectRandom _transforms;

						private _position = if ((_chosenTransforms select 0) isEqualType objNull) then {
							getPosASL (_chosenTransforms select 0);
						}
						else {
							_chosenTransforms select 0;
						};

						if !([_chosenTransforms, missionNamespace getVariable [_spawnerCount, 0]] call _condition) then {
							continue;
						};

						private _spawnValid = true;

						if _validatePosition then {
							{
								if ([_x, _position, _x, 1, _minimumPlayerDistance, 0, objNull] call KH_fnc_getPositionVisibility) then {
									_spawnValid = false;
									break;
								};
							} forEach KH_var_allPlayerUnits;
						};

						if !_spawnValid then {
							continue;
						};

						private _rotation = if ((count _chosenTransforms) > 1) then {
							if ((_chosenTransforms select 1) isEqualType objNull) then {
								private _chosenEntity = _chosenTransforms select 1;
								[vectorDir _chosenEntity, vectorUp _chosenEntity];
							}
							else {
								if ((_chosenTransforms select 1) isEqualTypeAll []) then {
									private _vectors = _chosenTransforms select 1;
									[_vectors select 0, _vectors select 1];
								}
								else {
									_chosenTransforms select 1;
								};
							};
						}
						else {
							[[0, 1, 0], [0, 0, 1]];
						};

						_position vectorAdd [random [(_radius select 0), 0, (_negativeRadius select 0)], random [(_radius select 1), 0, (_negativeRadius select 1)], random [(_radius select 2), 0, (_negativeRadius select 2)]];

						if !([_position, missionNamespace getVariable [_spawnerCount, 0]] call _condition) then {
							continue;
						};

						private _object = createSimpleObject [selectRandom _entityTypes, _position, _local];

						if (_rotation isEqualTypeAll []) then {
							_object setVectorDirAndUp _rotation;
						}
						else {
							_object setRotationEuler _rotation;
						};

						[_object, _chosenTransforms, _position] call _init;
						_spawnedEntities pushBack _object;
					};
				};

				case "VEHICLE": {
					private _placementMode = _type param [1, "CAN_COLLIDE", [""]];

					for "_i" from 1 to _amount do {
						private _chosenTransforms = selectRandom _transforms;

						private _position = if ((_chosenTransforms select 0) isEqualType objNull) then {
							(_chosenTransforms select 0) modelToWorld [0, 0, 0];
						}
						else {
							_chosenTransforms select 0;
						};

						if !([_chosenTransforms, missionNamespace getVariable [_spawnerCount, 0]] call _condition) then {
							continue;
						};

						private _spawnValid = true;

						if _validatePosition then {
							{
								if ([_x, AGLToASL _position, _x, 1, _minimumPlayerDistance, 0, objNull] call KH_fnc_getPositionVisibility) then {
									_spawnValid = false;
									break;
								};
							} forEach KH_var_allPlayerUnits;
						};

						if !_spawnValid then {
							continue;
						};

						private _rotation = if ((count _chosenTransforms) > 1) then {
							if ((_chosenTransforms select 1) isEqualType objNull) then {
								private _chosenEntity = _chosenTransforms select 1;
								[vectorDir _chosenEntity, vectorUp _chosenEntity];
							}
							else {
								if ((_chosenTransforms select 1) isEqualTypeAll []) then {
									private _vectors = _chosenTransforms select 1;
									[_vectors select 0, _vectors select 1];
								}
								else {
									_chosenTransforms select 1;
								};
							};
						}
						else {
							[[0, 1, 0], [0, 0, 1]];
						};

						_position vectorAdd [random [(_radius select 0), 0, (_negativeRadius select 0)], random [(_radius select 1), 0, (_negativeRadius select 1)], random [(_radius select 2), 0, (_negativeRadius select 2)]];
						
						if !([_position, missionNamespace getVariable [_spawnerCount, 0]] call _condition) then {
							continue;
						};

						private _vehicle = createVehicle [selectRandom _entityTypes, _position, [], 0, _placementMode];

						if (_rotation isEqualTypeAll []) then {
							_vehicle setVectorDirAndUp _rotation;
						}
						else {
							_vehicle setRotationEuler _rotation;
						};

						createVehicleCrew _vehicle;
						[_vehicle, _chosenTransforms, _position] call _init;
						_spawnedEntities pushBack _vehicle;
					};
				};
			};

			missionNamespace setVariable [_spawnerCount, (missionNamespace getVariable [_spawnerCount, 0]) + (count _spawnedEntities)];

			{
				if _countKilled then {
					[
						["ENTITY", _x, "REMOTE"],
						"Killed",
						[],
						{
							params ["_entity"];
							["KH_eve_spawnedEntityTerminated", []] call CBA_fnc_localEvent;
							[_handlerId] call KH_fnc_removeHandler;
						}
					] call KH_fnc_addEventHandler;
				};

				[
					["ENTITY", _x, "REMOTE"],
					"Deleted",
					[],
					{
						params ["_entity"];

						if (alive _entity) then {
							["KH_eve_spawnedEntityTerminated", []] call CBA_fnc_localEvent;
						};

						[_handlerId] call KH_fnc_removeHandler;
					}
				] call KH_fnc_addEventHandler;
			} forEach _spawnedEntities;

			{
				_x addCuratorEditableObjects [_spawnedEntities, true];
			} forEach allCurators;
		};
	},
	true,
	_interval,
	false
] call KH_fnc_execute;

[_entityHandler, _spawnHandler];