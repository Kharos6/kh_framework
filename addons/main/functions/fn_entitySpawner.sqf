params ["_entityTypes", "_transforms", "_radius", "_amount", "_maximum", "_condition", "_initialization", "_placementMode", "_type", "_interval"];
private _id = [missionNamespace, "KH_var_entitySpawner", "ACTIVE", false] call KH_fnc_atomicVariable;
private _spawnerCount = format [missionNamespace, "KH_var_entitySpawnerCount", 0, false] call KH_fnc_atomicVariable;

private _entityHandler = [
	"KH_eve_spawnedEntityTerminated", 
	{
		_args params ["_spawnerCount"];
		missionNamespace setVariable [_spawnerCount, (missionNamespace getVariable [_spawnerCount, 0]) - 1];
	}, 
	[_spawnerCount]
] call CBA_fnc_addEventHandlerArgs;

[
	{
		private _id = _args select 10;
		private _idState = missionNamespace getVariable [_id, "ACTIVE"];

		if (_idState != "INACTIVE") then {
			switch true do {
				case (_idState == "ACTIVE"): {
					_args params ["_entityTypes", "_transforms", "_radius", "_amount", "_maximum", "_condition", "_initialization", "_placementMode", "_type", "_spawnerCount"];

					if ((missionNamespace getVariable [_spawnerCount, 0]) <= _maximum) then {
						if ([_transforms, missionNamespace getVariable [_spawnerCount, 0]] call _condition) then {
							private _spawnedEntities = [];
							private _entityType = _type select 0;

							switch true do {
								case (_entityType == "UNIT"): {
									private _side = _type select 1;
									private _group = createGroup [_side, true];

									{
										private _chosenTransforms = selectRandom _transforms;
										private _position = [];
										private _rotation = [];

										if ((_chosenTransforms select 0) isEqualType objNull) then {
											_position = [_chosenTransforms select 0, "AGL", []] call KH_fnc_getPosition;
										}
										else {
											_position = _chosenTransforms select 0;
										};

										if ((_chosenTransforms select 1) isEqualType objNull) then {
											_rotation = [_chosenTransforms select 1, objNull] call KH_fnc_getRotation;
										}
										else {
											_rotation = _chosenTransforms select 1;
										};
										
										private _chosenRadius = _radius select (_transforms find _chosenTransforms);
										_position vectorAdd [random (_chosenRadius select 0), random (_chosenRadius select 1), random (_chosenRadius select 2)];
										private _unit = _group createUnit [selectRandom _entityTypes, _position, [], 0, _placementMode];
										[_unit, _rotation, false] call KH_fnc_setRotation;
										[_unit] call _initialization;
										_spawnedEntities pushBack _unit;
									} forEach _amount;
								};

								case (_entityType == "AGENT"): {
									{
										private _chosenTransforms = selectRandom _transforms;
										private _position = [];
										private _rotation = [];

										if ((_chosenTransforms select 0) isEqualType objNull) then {
											_position = [_chosenTransforms select 0, "AGL", []] call KH_fnc_getPosition;
										}
										else {
											_position = _chosenTransforms select 0;
										};

										if ((_chosenTransforms select 1) isEqualType objNull) then {
											_rotation = [_chosenTransforms select 1, objNull] call KH_fnc_getRotation;
										}
										else {
											_rotation = _chosenTransforms select 1;
										};

										private _chosenRadius = _radius select (_transforms find _chosenTransforms);
										_position vectorAdd [random (_chosenRadius select 0), random (_chosenRadius select 1), random (_chosenRadius select 2)];
										private _agent = createAgent [selectRandom _entityTypes, _position, [], 0, _placementMode];
										[_agent, _rotation, false] call KH_fnc_setRotation;
										[_agent] call _initialization;
										_spawnedEntities pushBack _agent;
									} forEach _amount;
								};

								case (_entityType == "OBJECT"): {
									{
										private _chosenTransforms = selectRandom _transforms;
										private _position = [];
										private _rotation = [];

										if ((_chosenTransforms select 0) isEqualType objNull) then {
											_position = [_chosenTransforms select 0, "AGL", []] call KH_fnc_getPosition;
										}
										else {
											_position = _chosenTransforms select 0;
										};

										if ((_chosenTransforms select 1) isEqualType objNull) then {
											_rotation = [_chosenTransforms select 1, objNull] call KH_fnc_getRotation;
										}
										else {
											_rotation = _chosenTransforms select 1;
										};

										private _chosenRadius = _radius select (_transforms find _chosenTransforms);
										_position vectorAdd [random (_chosenRadius select 0), random (_chosenRadius select 1), random (_chosenRadius select 2)];
										private _object = createVehicle [selectRandom _entityTypes, _position, [], 0, _placementMode];
										[_object, _rotation, false] call KH_fnc_setRotation;
										[_object] call _initialization;
										_spawnedEntities pushBack _object;
									} forEach _amount;
								};

								case (_entityType == "LOCAL_OBJECT"): {
									{
										private _chosenTransforms = selectRandom _transforms;
										private _position = [];
										private _rotation = [];

										if ((_chosenTransforms select 0) isEqualType objNull) then {
											_position = [_chosenTransforms select 0, "AGL", []] call KH_fnc_getPosition;
										}
										else {
											_position = _chosenTransforms select 0;
										};

										if ((_chosenTransforms select 1) isEqualType objNull) then {
											_rotation = [_chosenTransforms select 1, objNull] call KH_fnc_getRotation;
										}
										else {
											_rotation = _chosenTransforms select 1;
										};

										private _chosenRadius = _radius select (_transforms find _chosenTransforms);
										_position vectorAdd [random (_chosenRadius select 0), random (_chosenRadius select 1), random (_chosenRadius select 2)];
										private _object = createVehicleLocal [selectRandom _entityTypes, _position, [], 0, _placementMode];
										[_object, _rotation, false] call KH_fnc_setRotation;
										[_object] call _initialization;
										_spawnedEntities pushBack _object;
									} forEach _amount;
								};

								case (_entityType == "SIMPLE_OBJECT"): {
									{
										private _chosenTransforms = selectRandom _transforms;
										private _position = [];
										private _rotation = [];

										if ((_chosenTransforms select 0) isEqualType objNull) then {
											_position = [_chosenTransforms select 0, "ASL", []] call KH_fnc_getPosition;
										}
										else {
											_position = _chosenTransforms select 0;
										};

										if ((_chosenTransforms select 1) isEqualType objNull) then {
											_rotation = [_chosenTransforms select 1, objNull] call KH_fnc_getRotation;
										}
										else {
											_rotation = _chosenTransforms select 1;
										};

										private _chosenRadius = _radius select (_transforms find _chosenTransforms);
										_position vectorAdd [random (_chosenRadius select 0), random (_chosenRadius select 1), random (_chosenRadius select 2)];
										private _object = createSimpleObject [selectRandom _entityTypes, _position, false];
										[_object, _rotation, false] call KH_fnc_setRotation;
										[_object] call _initialization;
										_spawnedEntities pushBack _object;
									} forEach _amount;
								};

								case (_entityType == "LOCAL_SIMPLE_OBJECT"): {
									{
										private _chosenTransforms = selectRandom _transforms;
										private _position = [];
										private _rotation = [];

										if ((_chosenTransforms select 0) isEqualType objNull) then {
											_position = [_chosenTransforms select 0, "ASL", []] call KH_fnc_getPosition;
										}
										else {
											_position = _chosenTransforms select 0;
										};

										if ((_chosenTransforms select 1) isEqualType objNull) then {
											_rotation = [_chosenTransforms select 1, objNull] call KH_fnc_getRotation;
										}
										else {
											_rotation = _chosenTransforms select 1;
										};

										private _chosenRadius = _radius select (_transforms find _chosenTransforms);
										_position vectorAdd [random (_chosenRadius select 0), random (_chosenRadius select 1), random (_chosenRadius select 2)];
										private _object = createSimpleObject [selectRandom _entityTypes, _position, true];
										[_object, _rotation, false] call KH_fnc_setRotation;
										[_object] call _initialization;
										_spawnedEntities pushBack _object;
									} forEach _amount;
								};

								case (_entityType == "VEHICLE"): {
									{
										private _chosenTransforms = selectRandom _transforms;
										private _position = [];
										private _rotation = [];

										if ((_chosenTransforms select 0) isEqualType objNull) then {
											_position = [_chosenTransforms select 0, "AGL", []] call KH_fnc_getPosition;
										}
										else {
											_position = _chosenTransforms select 0;
										};

										if ((_chosenTransforms select 1) isEqualType objNull) then {
											_rotation = [_chosenTransforms select 1, objNull] call KH_fnc_getRotation;
										}
										else {
											_rotation = _chosenTransforms select 1;
										};

										private _chosenRadius = _radius select (_transforms find _chosenTransforms);
										_position vectorAdd [random (_chosenRadius select 0), random (_chosenRadius select 1), random (_chosenRadius select 2)];
										private _vehicle = createVehicle [selectRandom _entityTypes, _position, [], 0, _placementMode];
										[_vehicle, _rotation, false] call KH_fnc_setRotation;
										createVehicleCrew _vehicle;
										[_vehicle] call _initialization;
										_spawnedEntities pushBack _vehicle;
									} forEach _amount;
								};
							};

							[missionNamespace, _spawnerCount, count _spawnedEntities, false] call KH_fnc_atomicValue;

							{
								_x addEventHandler [
									"Killed", 
									{
										params ["_entity"];
										["KH_eve_spawnedEntityTerminated", []] call CBA_fnc_localEvent;
										_entity removeEventHandler [_thisEvent, _thisEventHandler];
									}
								];

								_x addEventHandler [
									"Deleted", 
									{
										params ["_entity"];

										if (alive _entity) then {
											["KH_eve_spawnedEntityTerminated", []] call CBA_fnc_localEvent;
										};

										_entity removeEventHandler [_thisEvent, _thisEventHandler];
									}
								];

								_x addEventHandler [
									"Local", 
									{
										params ["_entity"];
										["KH_eve_spawnedEntityTerminated", []] call CBA_fnc_localEvent;
										_entity removeEventHandler [_thisEvent, _thisEventHandler];
									}
								];
							} forEach _spawnedEntities;
						};
					}
					else {
						if !(_args select 12) then {
							["KH_eve_spawnedEntityTerminated", _entityHandler] call CBA_fnc_removeEventHandler;
							[_handle] call CBA_fnc_removePerFrameHandler;
						};
					};
				};

				case (_idState == "TERMINATE"): {
					private _entityHandler = _args select 10;
					["KH_eve_spawnedEntityTerminated", _entityHandler] call CBA_fnc_removeEventHandler;
					[_handle] call CBA_fnc_removePerFrameHandler;
				};		
			};
		};
	},
	_interval,
	[_entityTypes, _transforms, _radius, _maximum, _amount, _condition, _initialization, _placementMode, _type, _spawnerCount, _id, _entityHandler]
] call CBA_fnc_addPerFrameHandler;

["PRIVATE_HANDLER", _id, clientOwner];