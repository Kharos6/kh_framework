params ["_vehicle", "_movementData", ["_firingData", []], ["_disableDamage", true], ["_endPosition", objNull]];

if (_movementData isEqualType "") then { 
	_movementData = parseSimpleArray (preprocessFile _movementData);
};

if (_firingData isEqualType "") then { 
	_firingData = parseSimpleArray (preprocessFile _firingData);
};

private _driver = driver _vehicle;
private _endMove = [];

if (_endPosition isEqualType objNull) then {
	if !(isNull _endPosition) then {
		_endMove = getPosATL _endPosition;
	};
}
else {
	_endMove = _endPosition;
};

if _disableDamage then {
	_vehicle allowDamage false;
};

_vehicle engineOn true;

private _movementSequence = [_vehicle, _movementData, [], _disableDamage] spawn BIS_fnc_UnitPlay;
private _firingSequence = [_vehicle, _firingData, _disableDamage] spawn BIS_fnc_UnitPlayFiring;

[
	{
		params ["_movementSequence", "_firingSequence"];
		((scriptDone _movementSequence) && (scriptDone _firingSequence));
	}, 
	{
		private _vehicle = _this select 2;
		private _disableDamage = _this select 3;
		private _endMove = _this select 4;
		private _driver = _this select 5;
		
		if _disableDamage then {
			_vehicle allowDamage true;
		};
		
		if (_endMove isNotEqualTo []) then {
			[
				[_driver, _endMove],
				{
					params ["_driver", "_endMove"];
					_driver doMove _endMove;
				},
				_driver,
				"THIS_FRAME"
			] call KH_fnc_execute;
		};
	}, 
	[_movementSequence, _firingSequence, _vehicle, _disableDamage, _endMove, _driver]
] call CBA_fnc_waitUntilAndExecute;

[_movementSequence, _firingSequence];