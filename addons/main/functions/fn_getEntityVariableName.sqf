params [["_entity", objNull, [objNull, teamMemberNull, grpNull]], ["_generateIfEmpty", true, [true, ""]]];

if (_entity isEqualType teamMemberNull) then {
	_entity = agent _entity;
};

private _variableName = "";

if (_entity isEqualType objNull) then {
	_variableName = vehicleVarName _entity;

	if (isNil {missionNamespace getVariable _variableName;}) then {
		if (_variableName isNotEqualTo "") then {
			[_entity, _variableName] call KH_fnc_setEntityVariableName;
		}
		else {
			_variableName = hashValue _entity;

			if (isNil {missionNamespace getVariable _variableName;}) then {
				if (_generateIfEmpty isEqualTo true) then {
					_variableName = [_entity, ""] call KH_fnc_setEntityVariableName;
				}
				else {
					if (_generateIfEmpty isEqualTo false) then {
						_variableName = "";
					}
					else {
						_variableName = [_entity, _generateIfEmpty] call KH_fnc_setEntityVariableName;
					};
				};
			};
		};
	};
}
else {
	_variableName = [side _entity, groupId _entity] joinString "_";

	if (isNil {missionNamespace getVariable _variableName;}) then {
		if (_generateIfEmpty isEqualTo true) then {
			_variableName = [_entity, ""] call KH_fnc_setEntityVariableName;
		}
		else {
			if (_generateIfEmpty isNotEqualTo false) then {
				_variableName = [_entity, _generateIfEmpty] call KH_fnc_setEntityVariableName;
			};
		};
	};
};

_variableName;