params [
	["_identifier", "", [""]], 
	["_objects", (allMissionObjects "") select {!(_x isKindOf "Man") && !(_x isKindOf "Logic") && !(_x isKindOf "EmptyDetector") && (((typeOf _x) select [0, 1]) isNotEqualTo "#");}, []], 
	["_saveUndefined", false, [true]]
];

if (_identifier isEqualTo "") exitWith {
	[createHashMap, []];
};

private _persistencyNamespace = [_identifier, "persistency"] joinString "_";
private _currentEntries = _persistencyNamespace readKhData ["objectPersistency", createHashMap];
private _currentUndefinedEntries = [];

{
	private _variableName = vehicleVarName _x;

	if ((_variableName isNotEqualTo "") && !(_x getVariable ["KH_var_generatedVariableName", false])) then {
		_currentEntries set [_variableName, [_x] call KH_fnc_getObjectAttributes];
	}
	else {
		if _saveUndefined then {
			if !(_x isNil "KH_var_edenId") then {
				_currentUndefinedEntries pushBack [_x getVariable "KH_var_edenId", [_x] call KH_fnc_getObjectAttributes];
			}
			else {
				_currentUndefinedEntries pushBack ([_x] call KH_fnc_getObjectAttributes);
			};
		};
	};
} forEach (_objects select {(_x getVariable ["KH_var_allowPersistency", true]) && (([_x] call KH_fnc_getEntityNetworkType) isEqualTo "GLOBAL");});

_persistencyNamespace writeKhData ["objectPersistency", _currentEntries];
_persistencyNamespace writeKhData [["undefinedObjectPersistency", toLowerANSI worldName] joinString "_", _currentUndefinedEntries];
[_currentEntries, _currentUndefinedEntries];