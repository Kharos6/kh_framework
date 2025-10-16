params [["_vehicle", objNull, [objNull]], ["_movementData", [], ["", []]], ["_firingData", [], ["", []]], ["_disableDamage", true, [true]], ["_endPosition", true, [true, [], objNull]]];

if (_movementData isEqualType "") then { 
	_movementData = parseSimpleArray (preprocessFile _movementData);
};

if (_firingData isEqualType "") then { 
	_firingData = parseSimpleArray (preprocessFile _firingData);
};

if (_endPosition isEqualType objNull) then {
	_endPosition = getPosATL _endPosition;
}
else {
	if (_endPosition isEqualType true) then {
		if !_endPosition then {
			_endPosition = getPosATL _vehicle;
		}
		else {
			_endPosition = (_movementData select -1) select 1;
		};
	};
};

if _disableDamage then {
	_vehicle allowDamage false;
};

_vehicle engineOn true;
private _movementSequence = [_vehicle, _movementData, [], _disableDamage] spawn BIS_fnc_UnitPlay;
private _firingSequence = [_vehicle, _firingData, _disableDamage] spawn BIS_fnc_UnitPlayFiring;

[
	[_movementSequence, _firingSequence, _vehicle, _disableDamage, _endPosition], 
	{
		(_this select [2]) params ["_vehicle", "_disableDamage", "_endPosition"];
		
		if _disableDamage then {
			_vehicle allowDamage true;
		};
		
		if (_endPosition isNotEqualTo []) then {
			(driver _vehicle) doMove _endPosition;
		};
	},
	true,
	{
		params ["_movementSequence", "_firingSequence"];
		((scriptDone _movementSequence) && (scriptDone _firingSequence));
	},
	false
] call KH_fnc_execute;

[_movementSequence, _firingSequence];