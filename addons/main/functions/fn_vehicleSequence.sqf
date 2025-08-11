params ["_vehicle", "_movementData", ["_firingData", []], ["_disableDamage", true], ["_endPosition", objNull]];

if (_movementData isEqualType "") then { 
	_movementData = parseSimpleArray (preprocessFile _movementData);
};

if (_firingData isEqualType "") then { 
	_firingData = parseSimpleArray (preprocessFile _firingData);
};

if (_endPosition isEqualType objNull) then {
	if !(isNull _endPosition) then {
		_endPosition = getPosATL _endPosition;
	};
};

if _disableDamage then {
	_vehicle allowDamage false;
};

private _driver = driver _vehicle;
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
		private _endPosition = _this select 4;
		private _driver = _this select 5;
		
		if _disableDamage then {
			_vehicle allowDamage true;
		};
		
		if (_endPosition isNotEqualTo []) then {
			[
				[_driver, _endPosition],
				{
					params ["_driver", "_endPosition"];
					_driver doMove _endPosition;
				},
				_driver,
				true,
				false
			] call KH_fnc_execute;
		};
	}, 
	[_movementSequence, _firingSequence, _vehicle, _disableDamage, _endPosition, _driver]
] call CBA_fnc_waitUntilAndExecute;

[_movementSequence, _firingSequence];