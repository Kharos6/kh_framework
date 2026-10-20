params [["_identifier", "", [""]], ["_units", KH_var_allMen, [[]]], ["_saveUndefined", false, [true]]];

if (_identifier isEqualTo "") exitWith {
	createHashMap;
};

private _persistencyId = ["unitPersistency_", _identifier] joinString "";
private _undefinedPersistencyId = [toLowerANSI worldName, "_undefinedUnitPersistency_", _identifier] joinString "";
private _currentEntries = KH_var_khDataNamespace readKhData [_persistencyId, createHashMap];
private _currentUndefinedEntries = [];

{
	if ((_x getVariable ["KH_var_allowPersistency", true]) && !(isAgent (teamMember _x))) then {
		private _variableName = vehicleVarName _x;

		if ((_variableName isNotEqualTo "") && !(_x getVariable ["KH_var_generatedVariableName", false])) then {
			_currentEntries set [_variableName, [_x] call KH_fnc_getUnitAttributes];
		}
		else {
			if _saveUndefined then {
				_currentUndefinedEntries pushBack ([_x] call KH_fnc_getUnitAttributes);
			};
		};
	};
} forEach (_units select {!(isPlayer _x);});

KH_var_khDataNamespace writeKhData [_persistencyId, _currentEntries];
KH_var_khDataNamespace writeKhData [_undefinedPersistencyId, _currentUndefinedEntries];
_currentEntries;