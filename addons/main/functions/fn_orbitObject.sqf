params ["_state", "_entity", "_startingPosition", "_centerObject", "_radius", "_speed", "_angularVelocity", "_height", ["_sineAmplitude", 0], ["_clockwise", true], ["_interval", 0]];

if _state then {
	if !(_entity getVariable ["KH_var_orbitObjectState", false]) then {
		_entity setVariable ["KH_var_orbitObjectState", true];
		private _angleArray = [0]; 
		private _angleStep = (_speed / _radius) / 60;
		private _sineAngleArray = [0];
		_entity setPosATL _startingPosition;

		if _clockwise then {
			_angleStep = -_angleStep;
		};

		[
			{
				_args params ["_entity", "_centerObject", "_radius", "_angleStep", "_angularVelocity", "_height", "_angleArray", "_sineAmplitude", "_sineAngleArray"];
				
				if (_entity getVariable ["KH_var_orbitObjectState", false]) then {
					_angleArray set [0, (_angleArray select 0) + _angleStep];
					_sineAngleArray set [0, (_sineAngleArray select 0) + 1];

					if ((_angleArray select 0) > 360) then {
							_angleArray set [0, (_angleArray select 0) - 360];
					} 
					else {
						if ((_angleArray select 0) < 0) then {
							_angleArray set [0, (_angleArray select 0) + 360];
						};
					};

                    if ((_sineAngleArray select 0) > 360) then {
                        _sineAngleArray set [0, (_sineAngleArray select 0) - 360];
                    };

					private _angle = _angleArray select 0;
					private _sineHeight = _height + (_sineAmplitude * (sin (_sineAngleArray select 0)));
					private _xOffset = _radius * (cos _angle);
					private _yOffset = _radius * (sin _angle);
					private _nextPosition = (getPosATL _centerObject) vectorAdd [_xOffset, _yOffset, 0];
					_nextPosition set [2, _sineHeight];
					private _currentPosition = getPosATL _entity;
					private _velocity = _nextPosition vectorDiff _currentPosition;
					_entity setVelocity _velocity;
					_entity setAngularVelocity _angularVelocity;
				} 
				else {
					[_handle] call CBA_fnc_removePerFrameHandler;
				};
			},
			_interval,
			[_entity, _centerObject, _radius, _angleStep, _angularVelocity, _height, _angleArray, _sineAmplitude, _sineAngleArray]
		] call CBA_fnc_addPerFrameHandler;
	};
}
else {
	_entity setVariable ["KH_var_orbitObjectState", false];
};

true;