params [["_identifier", "", [""]], ["_players", KH_var_allPlayerUnits, []]];

if (_identifier isEqualTo "") exitWith {
	createHashMap;
};

private _persistencyUidId = ["playerPersistencyUid_", _identifier] joinString "";
private _persistencyVariableNameId = ["playerPersistencyVariableName_", _identifier] joinString "";
private _currentEntriesUid = "khNamespace" readKhData [_persistencyUidId, createHashMap];
private _currentEntriesVariableName = "khNamespace" readKhData [_persistencyVariableNameId, createHashMap];

{
	if (_x getVariable ["KH_var_allowPersistency", true]) then {
		private _scores = getPlayerScores _x;
		_scores deleteAt 5;
		_currentEntriesUid set [getPlayerUID _x, [[_x] call KH_fnc_getUnitAttributes, _scores]];
		private _variableName = vehicleVarName _x;

		if ((_variableName isNotEqualTo "") && !(_x getVariable ["KH_var_generatedVariableName", false])) then {
			_currentEntriesVariableName set [_variableName, [[_x] call KH_fnc_getUnitAttributes, _scores]];
		};
	};
} forEach _players;

"khNamespace" writeKhData [_persistencyUidId, _currentEntriesUid];
"khNamespace" writeKhData [_persistencyVariableNameId, _currentEntriesVariableName];
[_currentEntriesUid, _currentEntriesVariableName];