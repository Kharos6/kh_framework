params [["_identifier", "", [""]], ["_units", allUnits, []]];

if (_identifier isEqualTo "") exitWith {
	createHashMap;
};

private _persistencyId = ["unitPersistency_", _identifier] joinString "";
private _currentEntries = "khNamespace" readKhData [_persistencyId, createHashMap];

{
	private _variableName = vehicleVarName _x;

	if ((_variableName isNotEqualTo "") && !(_x getVariable ["KH_var_generatedVariableName", false])) then {
		_currentEntries set [_variableName, [_x] call KH_fnc_getUnitAttributes];
	};
} forEach (_units select {!(isPlayer _x);});

"khNamespace" writeKhData [_persistencyId, _currentEntries];
_currentEntries;