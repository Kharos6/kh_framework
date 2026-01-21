params [["_identifier", "", [""]], ["_units", allUnits + allDeadMen, []]];

if (_identifier isEqualTo "") exitWith {
	createHashMap;
};

private _persistencyId = ["unitPersistency_", _identifier] joinString "";
private _currentEntries = KH_var_khDataNamespace readKhData [_persistencyId, createHashMap];

{
	if (_x getVariable ["KH_var_allowPersistency", true]) then {
		private _variableName = vehicleVarName _x;

		if ((_variableName isNotEqualTo "") && !(_x getVariable ["KH_var_generatedVariableName", false])) then {
			_currentEntries set [_variableName, [_x] call KH_fnc_getUnitAttributes];
		};
	};
} forEach (_units select {!(isPlayer _x);});

KH_var_khDataNamespace writeKhData [_persistencyId, _currentEntries];
_currentEntries;