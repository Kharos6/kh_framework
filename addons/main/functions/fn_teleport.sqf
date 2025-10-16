params [
	["_entity", objNull, [objNull]], 
	["_position", [0, 0, 0], [[], objNull]], 
	["_rotation", [0, 0, 0], [[], objNull]], 
	["_eject", true, [true]], 
	["_transition", 1, [0]],
	["_freefallHeight", -1, [0]], 
	["_init", {}, [{}]]
];

if (_position isEqualType objNull) then {
	_position = getPosATL _position;
};

if (_rotation isEqualType objNull) then {
	_rotation = [vectorDir _rotation, vectorUp _rotation];
};

if (_freefallHeight isNotEqualTo 0) then {
	[
		[_entity, _freefallHeight],
		{
			params ["_entity", "_freefallHeight"];
			_entity setUnitFreefallHeight _freefallHeight;
		},
		_entity,
		true,
		false
	] call KH_fnc_execute;
};

if (_transition isNotEqualTo 0) then {
	_transition = _transition / 2;

	if (isPlayer _entity) then {
		[
			["", "KH_ResourceKHDisplay", [_transition, 0, _transition], [0, 0, 0, 0], [0, 0, 100, 100], [0, 0, 0]],
			"KH_fnc_draw2d",
			_entity,
			true,
			false
		] call KH_fnc_execute;
	};

	[
		[_entity, _position, _rotation, _eject, _init],
		{
			params ["_entity", "_position", "_rotation", "_eject", "_init"];

			if _eject then {
				moveOut _entity;
				
				[
					[_entity, _position, _rotation, _init],
					{
						params ["_entity", "_position", "_rotation", "_init"];
						_entity setPosATL _position;
						_entity setVectorDirAndUp _rotation;
						[_entity] call _init;
					},
					true,
					[
						{
							params ["_entity"];
							(isNull (objectParent _entity));
						},
						true,
						1,
						0,
						true,
						true
					],
					false
				] call KH_fnc_execute;
			}
			else {
				_entity setPosATL _position;
				_entity setVectorDirAndUp _rotation;
				[_entity] call _init;
			};
		},
		_entity,
		str _transition,
		false
	] call KH_fnc_execute;
}
else {
	if _eject then {
		moveOut _entity;
		
		[
			[_entity, _position, _rotation, _init],
			{
				params ["_entity", "_position", "_rotation", "_init"];
				_entity setPosATL _position;
				_entity setVectorDirAndUp _rotation;
				[_entity] call _init;
			},
			_entity,
			[
				{
					params ["_entity"];
					(isNull (objectParent _entity));
				},
				true,
				1,
				0,
				true,
				true
			],
			false
		] call KH_fnc_execute;
	}
	else {
		_entity setPosATL _position;

		[
			[_entity, _rotation], 
			{
				params ["_entity", "_rotation"];
				_entity setVectorDirAndUp _rotation;
			}, 
			_entity,
			true, 
			false
		] call KH_fnc_execute;

		[[_entity], _init, _entity, true, false] call KH_fnc_execute;
	};
};

nil;