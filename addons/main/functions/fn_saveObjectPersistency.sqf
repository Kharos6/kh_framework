params [["_identifier", "", [""]], ["_objects", entities [[], ["Man"], false, true], []]];

if (_identifier isEqualTo "") exitWith {
	createHashMap;
};

private _persistencyId = ["objectPersistency_", _identifier] joinString "";
private _currentEntries = "khNamespace" readKhData [_persistencyId, createHashMap];

{
	if (_x getVariable ["KH_var_allowPersistency", true]) then {
		private _variableName = vehicleVarName _x;

		if ((_variableName isNotEqualTo "") && !(_x getVariable ["KH_var_generatedVariableName", false])) then {
			_currentEntries set [_variableName, [_x] call KH_fnc_getObjectAttributes];
		};
	};
} forEach _objects;

"khNamespace" writeKhData [_persistencyId, _currentEntries];
_currentEntries;