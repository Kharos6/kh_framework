params [
	["_entityTypes", [], [[]]], 
	["_transforms", [], [[]]], 
	["_radiuses", [], [[]]], 
	["_amount", 1, [0]], 
	["_maximum", 1, [0]], 
	["_condition", {}, [{}]], 
	["_init", {}, [{}]], 
	["_type", [], [[]]],
	["_interval", 1, [0]]
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
	[_entityTypes, _transforms, _radiuses, _amount, _maximum, _condition, _init, _type, _spawnerCount],
	{
		params ["_entityTypes", "_transforms", "_radiuses", "_amount", "_maximum", "_condition", "_init", "_type", "_spawnerCount"];

		if ((missionNamespace getVariable [_spawnerCount, 0]) < _maximum) then {
			if ([missionNamespace getVariable [_spawnerCount, 0]] call _condition) then {
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
								_position = (_chosenTransforms select 0) modelToWorld [0, 0, 0];
							}
							else {
								_position = _chosenTransforms select 0;
							};

							private _rotation = if ((_chosenTransforms select 1) isEqualType objNull) then {
								private _chosenEntity = _chosenTransforms select 1;
								_rotation = [vectorDir _chosenEntity, vectorUp _chosenEntity];
							}
							else {
								private _vectors = _chosenTransforms select 1;
								_rotation = [_vectors select 0, _vectors select 1];
							};
							
							private _chosenRadius = _radiuses select (_transforms find _chosenTransforms);
							_position vectorAdd [random (_chosenRadius select 0), random (_chosenRadius select 1), random (_chosenRadius select 2)];
							private _unit = _group createUnit [selectRandom _entityTypes, _position, [], 0, _placementMode];
							_unit setVectorDirAndUp _rotation;
							[_unit, _chosenTransforms, _position] call _init;
							_spawnedEntities pushBack _unit;
						};
					};

					case "AGENT": {
						private _placementMode = _type param [1, "CAN_COLLIDE", [""]];

						for "_i" from 1 to _amount do {
							private _chosenTransforms = selectRandom _transforms;

							private _position = if ((_chosenTransforms select 0) isEqualType objNull) then {
								_position = (_chosenTransforms select 0) modelToWorld [0, 0, 0];
							}
							else {
								_position = _chosenTransforms select 0;
							};

							private _rotation = if ((_chosenTransforms select 1) isEqualType objNull) then {
								private _chosenEntity = _chosenTransforms select 1;
								_rotation = [vectorDir _chosenEntity, vectorUp _chosenEntity];
							}
							else {
								private _vectors = _chosenTransforms select 1;
								_rotation = [_vectors select 0, _vectors select 1];
							};

							private _chosenRadius = _radiuses select (_transforms find _chosenTransforms);
							_position vectorAdd [random (_chosenRadius select 0), random (_chosenRadius select 1), random (_chosenRadius select 2)];
							private _agent = createAgent [selectRandom _entityTypes, _position, [], 0, _placementMode];
							_agent setVectorDirAndUp _rotation;
							[_agent, _chosenTransforms, _position] call _init;
							_spawnedEntities pushBack _agent;
						};
					};

					case "OBJECT": {
						(_type select [1]) params [["_placementMode", "CAN_COLLIDE", [""]], ["_local", false, [true]]];

						for "_i" from 1 to _amount do {
							private _chosenTransforms = selectRandom _transforms;

							private _position = if ((_chosenTransforms select 0) isEqualType objNull) then {
								_position = (_chosenTransforms select 0) modelToWorld [0, 0, 0];
							}
							else {
								_position = _chosenTransforms select 0;
							};

							private _rotation = if ((_chosenTransforms select 1) isEqualType objNull) then {
								private _chosenEntity = _chosenTransforms select 1;
								_rotation = [vectorDir _chosenEntity, vectorUp _chosenEntity];
							}
							else {
								private _vectors = _chosenTransforms select 1;
								_rotation = [_vectors select 0, _vectors select 1];
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

							_object setVectorDirAndUp _rotation;
							[_object, _chosenTransforms, _position] call _init;
							_spawnedEntities pushBack _object;
						};
					};

					case "SIMPLE_OBJECT": {
						private _local = _type param [1, false, [true]];

						for "_i" from 1 to _amount do {
							private _chosenTransforms = selectRandom _transforms;

							private _position = if ((_chosenTransforms select 0) isEqualType objNull) then {
								_position = (_chosenTransforms select 0) modelToWorld [0, 0, 0];
							}
							else {
								_position = _chosenTransforms select 0;
							};

							private _rotation = if ((_chosenTransforms select 1) isEqualType objNull) then {
								private _chosenEntity = _chosenTransforms select 1;
								_rotation = [vectorDir _chosenEntity, vectorUp _chosenEntity];
							}
							else {
								private _vectors = _chosenTransforms select 1;
								_rotation = [_vectors select 0, _vectors select 1];
							};

							private _chosenRadius = _radiuses select (_transforms find _chosenTransforms);
							_position vectorAdd [random (_chosenRadius select 0), random (_chosenRadius select 1), random (_chosenRadius select 2)];
							private _object = createSimpleObject [selectRandom _entityTypes, _position, _local];
							_object setVectorDirAndUp _rotation;
							[_object, _chosenTransforms, _position] call _init;
							_spawnedEntities pushBack _object;
						};
					};

					case "VEHICLE": {
						private _placementMode = _type param [1, "CAN_COLLIDE", [""]];

						for "_i" from 1 to _amount do {
							private _chosenTransforms = selectRandom _transforms;

							private _position = if ((_chosenTransforms select 0) isEqualType objNull) then {
								_position = (_chosenTransforms select 0) modelToWorld [0, 0, 0];
							}
							else {
								_position = _chosenTransforms select 0;
							};

							private _rotation = if ((_chosenTransforms select 1) isEqualType objNull) then {
								private _chosenEntity = _chosenTransforms select 1;
								_rotation = [vectorDir _chosenEntity, vectorUp _chosenEntity];
							}
							else {
								private _vectors = _chosenTransforms select 1;
								_rotation = [_vectors select 0, _vectors select 1];
							};

							private _chosenRadius = _radiuses select (_transforms find _chosenTransforms);
							_position vectorAdd [random (_chosenRadius select 0), random (_chosenRadius select 1), random (_chosenRadius select 2)];
							private _vehicle = createVehicle [selectRandom _entityTypes, _position, [], 0, _placementMode];
							_vehicle setVectorDirAndUp _rotation;
							createVehicleCrew _vehicle;
							[_vehicle, _chosenTransforms, _position] call _init;
							_spawnedEntities pushBack _vehicle;
						};
					};
				};

				missionNamespace setVariable [_spawnerCount, (missionNamespace getVariable [_spawnerCount, 0]) + (count _spawnedEntities)];

				{
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
		};
	},
	true,
	_interval,
	false
] call KH_fnc_execute;

[_entityHandler, _spawnHandler];