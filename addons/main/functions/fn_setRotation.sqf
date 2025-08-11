params ["_entity", "_rotation", ["_relative", false]];

switch (typeName _rotation) do {
	case "ARRAY": {
		private _aroundX = (360 - (_rotation select 0)) - 360;
		private _aroundY = (360 - (_rotation select 1)) - 360;
		private _aroundZ = (360 - (_rotation select 2)) - 360;

		if _relative then {
			private _originalRotation = [_entity, objNull] call KH_fnc_getRotation;
			_aroundX = (360 - ((_originalRotation select 0) + (_rotation select 0))) - 360;
			_aroundY = (360 - ((_originalRotation select 1) + (_rotation select 1))) - 360; 
			_aroundZ = (360 - ((_originalRotation select 2) + (_rotation select 2))) - 360;
		};

		private _dirX = 0;
		private _dirY = 1;
		private _dirZ = 0;
		private _upX = 0;
		private _upY = 0;
		private _upZ = 1;

		if (_aroundX != 0) then {
			_dirY = cos _aroundX;
			_dirZ = sin _aroundX;
			_upY = -(sin _aroundX);
			_upZ = cos _aroundX;
		};

		if (_aroundY != 0) then {
			_dirX = _dirZ * (sin _aroundY);
			_dirZ = _dirZ * (cos _aroundY);
			_upX = _upZ * (sin _aroundY);
			_upZ = _upZ * (cos _aroundY);
		};

		if (_aroundZ != 0) then {
			_dirXTemp = _dirX;
			_dirX = (_dirXTemp * (cos _aroundZ)) - (_dirY * (sin _aroundZ));
			_dirY = (_dirY * (cos _aroundZ)) + (_dirXTemp * (sin _aroundZ));
			_upXTemp = _upX;
			_upX = (_upXTemp * (cos _aroundZ)) - (_upY * (sin _aroundZ));
			_upY = (_upY * (cos _aroundZ)) + (_upXTemp * (sin _aroundZ));
		};

		private _dir = [_dirX, _dirY, _dirZ];
		private _up = [_upX, _upY, _upZ];

		[
			[_entity, _dir, _up],
			{
				params ["_entity", "_dir", "_up"];
				_entity setVectorDirAndUp [_dir, _up];
			},
			_entity,
			true
		] call KH_fnc_execute;

		true;
	};

	case "OBJECT": {
		private _entityRotation = [_rotation, objNull] call KH_fnc_getRotation;
		private _aroundX = (360 - (_entityRotation select 0)) - 360;
		private _aroundY = (360 - (_entityRotation select 1)) - 360;
		private _aroundZ = (360 - (_entityRotation select 2)) - 360;

		if _relative then {
			private _rotations = [_entity, _rotation] call KH_fnc_getRotation;
			_aroundX = (360 - (_rotations select 0)) - 360;
			_aroundY = 0; 
			_aroundZ = (360 - (_rotations select 2)) - 360;
		};

		private _dirX = 0;
		private _dirY = 1;
		private _dirZ = 0;
		private _upX = 0;
		private _upY = 0;
		private _upZ = 1;

		if (_aroundX != 0) then {
			_dirY = cos _aroundX;
			_dirZ = sin _aroundX;
			_upY = -(sin _aroundX);
			_upZ = cos _aroundX;
		};

		if (_aroundY != 0) then {
			_dirX = _dirZ * (sin _aroundY);
			_dirZ = _dirZ * (cos _aroundY);
			_upX = _upZ * (sin _aroundY);
			_upZ = _upZ * (cos _aroundY);
		};

		if (_aroundZ != 0) then {
			_dirXTemp = _dirX;
			_dirX = (_dirXTemp * (cos _aroundZ)) - (_dirY * (sin _aroundZ));
			_dirY = (_dirY * (cos _aroundZ)) + (_dirXTemp * (sin _aroundZ));
			_upXTemp = _upX;
			_upX = (_upXTemp * (cos _aroundZ)) - (_upY * (sin _aroundZ));
			_upY = (_upY * (cos _aroundZ)) + (_upXTemp * (sin _aroundZ));
		};

		private _dir = [_dirX, _dirY, _dirZ];
		private _up = [_upX, _upY, _upZ];

		[
			[_entity, _dir, _up],
			{
				params ["_entity", "_dir", "_up"];
				_entity setVectorDirAndUp [_dir, _up];
			},
			_entity,
			true
		] call KH_fnc_execute;

		true;
	};

	default {
		false;
	};
};