params [
	["_entity", objNull, [objNull]], 
	["_position", [0, 0, 0], [[], objNull]], 
	["_rotation", [0, 0, 0], [[], objNull]], 
	["_eject", true, [true]], 
	["_transition", 1, [0]],
	["_freefallHeight", -1, [0]], 
	["_init", {}, [{}]]
];

if (_freefallHeight isNotEqualTo 0) then {
	_entity setUnitFreefallHeight _freefallHeight;
};

if (_position isEqualType objNull) then {
	_position = getPosATL _position;
};

if (_rotation isEqualType objNull) then {
	_rotation = [vectorDir _rotation, vectorUp _rotation];
}
else {
	if (_rotation isEqualTypeAll 0) then {
		eulerToVector _rotation;
	};
};

private _useTransition = if (_transition isNotEqualTo 0) then {
	_transition = _transition / 2;
	["RscText", "", [_transition, true, _transition], [0, 0, 0, 1], [0, 0, 100, 100], false, [0, 0, 0]] call KH_fnc_draw2d;
	true;
}
else {
	false;
};

[
	[_entity, _position, _rotation, _eject, _init],
	{
		params ["_entity", "_position", "_rotation", "_eject", "_init"];

		if _eject then {
			[
				[_entity, _position, _rotation, _init],
				{
					params ["_entity", "_position", "_rotation", "_init"];

					if (isNull (objectParent _entity)) then {
						_entity setPosATL _position;

						if (_rotation isEqualTypeAll []) then {
							_entity setVectorDirAndUp _rotation;
						}
						else {
							_entity setRotationEuler _rotation;
						};

						[_entity] call _init;
						[_handlerId] call KH_fnc_removeHandler;
					}
					else {
						moveOut _entity;
					};
				},
				true,
				0,
				false
			] call KH_fnc_execute;
		}
		else {
			_entity setPosATL _position;

			if (_rotation isEqualTypeAll []) then {
				_entity setVectorDirAndUp _rotation;
			}
			else {
				_entity setRotationEuler _rotation;
			};

			[_entity] call _init;
		};
	},
	true,
	[true, str _transition] select _useTransition,
	false
] call KH_fnc_execute;

nil;