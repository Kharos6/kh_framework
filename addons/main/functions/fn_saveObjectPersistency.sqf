params [["_identifier", "", [""]], ["_objects", (allMissionObjects "") select {!(_x isKindOf "Man") && !(_x isKindOf "Logic") && !(_x isKindOf "EmptyDetector") && (((typeOf _x) select [0, 1]) isNotEqualTo "#");}, []], ["_saveUndefined", false, [true]]];

if (_identifier isEqualTo "") exitWith {
	createHashMap;
};

private _persistencyId = ["objectPersistency_", _identifier] joinString "";
private _undefinedPersistencyId = [toLowerANSI worldName, "_undefinedObjectPersistency_", _identifier] joinString "";
private _currentEntries = KH_var_khDataNamespace readKhData [_persistencyId, createHashMap];
private _currentUndefinedEntries = [];

{
	if (_x getVariable ["KH_var_allowPersistency", true]) then {
		private _variableName = vehicleVarName _x;

		if ((_variableName isNotEqualTo "") && !(_x getVariable ["KH_var_generatedVariableName", false])) then {
			_currentEntries set [_variableName, [_x] call KH_fnc_getObjectAttributes];
		}
		else {
			if _saveUndefined then {
				_currentUndefinedEntries pushBack ([_x] call KH_fnc_getObjectAttributes);
			};
		};
	};
} forEach _objects;

KH_var_khDataNamespace writeKhData [_persistencyId, _currentEntries];
KH_var_khDataNamespace writeKhData [_undefinedPersistencyId, _currentUndefinedEntries];
_currentEntries;