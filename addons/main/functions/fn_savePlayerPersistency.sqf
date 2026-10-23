params [["_identifier", "", [""]], ["_players", KH_var_allPlayerUnits, []], ["_saveDisconnected", true, [true]]];

if (_identifier isEqualTo "") exitWith {
	createHashMap;
};

private _persistencyNamespace = [_identifier, "persistency"] joinString "_";
private _currentEntriesUid = _persistencyNamespace readKhData ["playerPersistencyUid", createHashMap];
private _currentEntriesVariableName = _persistencyNamespace readKhData ["playerPersistencyVariableName", createHashMap];

{
	private _scores = getPlayerScores _x;
	_scores deleteAt 5;
	_currentEntriesUid set [getPlayerUID _x, [[_x] call KH_fnc_getUnitAttributes, _scores]];
	private _variableName = vehicleVarName _x;

	if ((_variableName isNotEqualTo "") && !(_x getVariable ["KH_var_generatedVariableName", false])) then {
		_currentEntriesVariableName set [_variableName, [[_x] call KH_fnc_getUnitAttributes, _scores]];
	};
} forEach (_players select {_x getVariable ["KH_var_allowPersistency", true];});

if _saveDisconnected then {
	{
		_y params ["_variableName", "_attributes", "_scores", "_allowPersistency", "_generatedVariableName"];

		if _allowPersistency then {
			_currentEntriesUid set [_x, [_attributes, _scores]];

			if ((_variableName isNotEqualTo "") && !_generatedVariableName) then {
				_currentEntriesVariableName set [_variableName, [_attributes, _scores]];
			};
		};
	} forEach KH_var_disconnectedPlayerStorage;
};

_persistencyNamespace writeKhData ["playerPersistencyUid", _currentEntriesUid];
_persistencyNamespace writeKhData ["playerPersistencyVariableName", _currentEntriesVariableName];
[_currentEntriesUid, _currentEntriesVariableName];