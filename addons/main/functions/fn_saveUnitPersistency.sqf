params [["_identifier", "", [""]], ["_units", KH_var_allMen, [[]]], ["_saveUndefined", false, [true]]];

if (_identifier isEqualTo "") exitWith {
	[createHashMap, []];
};

private _persistencyNamespace = [_identifier, "persistency"] joinString "_";
private _currentEntries = _persistencyNamespace readKhData ["unitPersistency", createHashMap];
private _currentUndefinedEntries = [];

{
	private _variableName = vehicleVarName _x;

	if ((_variableName isNotEqualTo "") && !(_x getVariable ["KH_var_generatedVariableName", false])) then {
		_currentEntries set [_variableName, [_x] call KH_fnc_getUnitAttributes];
	}
	else {
		if _saveUndefined then {
			if !(_x isNil "KH_var_edenId") then {
				_currentUndefinedEntries pushBack [_x getVariable "KH_var_edenId", [_x] call KH_fnc_getUnitAttributes];
			}
			else {
				_currentUndefinedEntries pushBack ([_x] call KH_fnc_getUnitAttributes);
			};
		};
	};
} forEach (_units select {(_x getVariable ["KH_var_allowPersistency", true]) && !(isPlayer _x) && !(isAgent (teamMember _x));});

_persistencyNamespace writeKhData ["unitPersistency", _currentEntries];
_persistencyNamespace writeKhData [["undefinedUnitPersistency", toLowerANSI worldName] joinString "_", _currentUndefinedEntries];
[_currentEntries, _currentUndefinedEntries];