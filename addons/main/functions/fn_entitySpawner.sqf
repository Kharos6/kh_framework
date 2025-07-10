params ["_entityTypes", "_transforms", "_radiuses", "_amount", "_maximum", "_condition", "_init", "_type", "_interval"];
private _id = [missionNamespace, "KH_var_entitySpawner", "ACTIVE", false] call KH_fnc_atomicVariable;
private _spawnerCount = [missionNamespace, "KH_var_entitySpawnerCount", 0, false] call KH_fnc_atomicVariable;

private _entityHandler = [
	"CBA",
	"KH_eve_spawnedEntityTerminated",
	[_spawnerCount],
	{
		_args params ["_spawnerCount"];
		missionNamespace setVariable [_spawnerCount, (missionNamespace getVariable [_spawnerCount, 0]) - 1];
	}
] call KH_fnc_addEventHandler;

[
	{
		private _id = _args select 10;
		private _idState = missionNamespace getVariable [_id, "ACTIVE"];

		if (_idState isNotEqualTo "INACTIVE") then {
			switch _idState do {
				case "ACTIVE": {
					_args params ["_entityTypes", "_transforms", "_radiuseses", "_amount", "_maximum", "_condition", "_init", "_type", "_spawnerCount"];

					if ((missionNamespace getVariable [_spawnerCount, 0]) <= _maximum) then {
						if ([_transforms, missionNamespace getVariable [_spawnerCount, 0], ["PRIVATE_HANDLER", _id, clientOwner]] call _condition) then {
							private _spawnedEntities = [];
							private _entityType = _type select 0;
							private _i = 1;

							switch _entityType do {
								case "UNIT": {
									private _placementMode = _type select 1;
									private _side = _type select 2;
									private _shareGroup = _type select 3;
									private _group = grpNull;

									if _shareGroup then {
										_group = createGroup [_side, true];
									};

									for "_i" from 1 to _amount do {
										if !_shareGroup then {
											_group = createGroup [_side, true];
										};

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
										
										private _chosenRadius = _radiuses select (_transforms find _chosenTransforms);
										_position vectorAdd [random (_chosenRadius select 0), random (_chosenRadius select 1), random (_chosenRadius select 2)];
										private _unit = _group createUnit [selectRandom _entityTypes, _position, [], 0, _placementMode];
										[_unit, _rotation, false] call KH_fnc_setRotation;
										[_unit, _chosenTransforms, _position] call _init;
										_spawnedEntities pushBack _unit;
									};
								};

								case "AGENT": {
									private _placementMode = _type select 1;

									for "_i" from 1 to _amount do {
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

										private _chosenRadius = _radiuses select (_transforms find _chosenTransforms);
										_position vectorAdd [random (_chosenRadius select 0), random (_chosenRadius select 1), random (_chosenRadius select 2)];
										private _agent = createAgent [selectRandom _entityTypes, _position, [], 0, _placementMode];
										[_agent, _rotation, false] call KH_fnc_setRotation;
										[_agent, _chosenTransforms, _position] call _init;
										_spawnedEntities pushBack _agent;
									};
								};

								case "OBJECT": {
									private _placementMode = _type select 1;
									private _local = _type select 2;

									for "_i" from 1 to _amount do {
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

										private _chosenRadius = _radiuses select (_transforms find _chosenTransforms);
										_position vectorAdd [random (_chosenRadius select 0), random (_chosenRadius select 1), random (_chosenRadius select 2)];
										private _object = objNull;
										
										if !_local then {
											_object = createVehicle [selectRandom _entityTypes, _position, [], 0, _placementMode];
										}
										else {
											_object = createVehicleLocal [selectRandom _entityTypes, _position, [], 0, _placementMode];
										};

										[_object, _rotation, false] call KH_fnc_setRotation;
										[_object, _chosenTransforms, _position] call _init;
										_spawnedEntities pushBack _object;
									};
								};

								case "SIMPLE_OBJECT": {
									private _local = _type select 1;

									for "_i" from 1 to _amount do {
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

										private _chosenRadius = _radiuses select (_transforms find _chosenTransforms);
										_position vectorAdd [random (_chosenRadius select 0), random (_chosenRadius select 1), random (_chosenRadius select 2)];
										private _object = createSimpleObject [selectRandom _entityTypes, _position, _local];
										[_object, _rotation, false] call KH_fnc_setRotation;
										[_object, _chosenTransforms, _position] call _init;
										_spawnedEntities pushBack _object;
									};
								};

								case "VEHICLE": {
									private _placementMode = _type select 1;

									for "_i" from 1 to _amount do {
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

										private _chosenRadius = _radiuses select (_transforms find _chosenTransforms);
										_position vectorAdd [random (_chosenRadius select 0), random (_chosenRadius select 1), random (_chosenRadius select 2)];
										private _vehicle = createVehicle [selectRandom _entityTypes, _position, [], 0, _placementMode];
										[_vehicle, _rotation, false] call KH_fnc_setRotation;
										createVehicleCrew _vehicle;
										[_vehicle, _chosenTransforms, _position] call _init;
										_spawnedEntities pushBack _vehicle;
									};
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

							{
								_x addCuratorEditableObjects [_spawnedEntities, true];
							} forEach allCurators;
						};
					}
					else {
						if !(_args select 12) then {
							[_entityHandler] call KH_fnc_removeEventHandler;
							[_handle] call CBA_fnc_removePerFrameHandler;
						};
					};
				};

				case "TERMINATE": {
					private _entityHandler = _args select 10;
					[_entityHandler] call KH_fnc_removeEventHandler;
					[_handle] call CBA_fnc_removePerFrameHandler;
				};		
			};
		};
	},
	_interval,
	[_entityTypes, _transforms, _radiuses, _amount, _maximum, _condition, _init, _type, _spawnerCount, _id, _entityHandler]
] call CBA_fnc_addPerFrameHandler;

["PRIVATE_HANDLER", _id, clientOwner];