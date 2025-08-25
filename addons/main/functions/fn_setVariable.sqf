params [
	["_namespace", missionNamespace, ["", missionNamespace, objNull, teamMemberNull, grpNull, locationNull, taskNull, controlNull, displayNull]], 
	"_name", 
	"_value", 
	["_scope", true, [true, 0, "", [], {}, objNull, teamMemberNull, grpNull, sideUnknown, locationNull]], 
	["_writeType", "OVERWRITE", [""]],
	["_persistent", false, [true]];
];

private _continue = true;
private _khData = false;

if (_namespace isEqualType "") then {
	_name = switch (typeName _name) do {
		case "OBJECT": {
			if (isPlayer _name) then {
				["player_", getPlayerUID _name] joinString "";
			}
			else {
				if ((vehicleVarName _name) isEqualTo "") then {
					if ((roleDescription _name) isEqualTo "") then {
						"";
					}
					else {
						roleDescription _name;
					};
				}
				else {
					vehicleVarName _name;
				};
			};
		};

		case "TEAM_MEMBER": {
			private _entity = agent _name;

			if (isPlayer _entity) then {
				getPlayerUID _entity;
			}
			else {
				if ((vehicleVarName _entity) isEqualTo "") then {
					if ((roleDescription _entity) isEqualTo "") then {
						"";
					}
					else {
						roleDescription _entity;
					};
				}
				else {
					vehicleVarName _entity;
				};
			};
		};

		case "GROUP": {
			groupId _name;
		};

		case "LOCATION": {
			name _name;
		};
		
		case "STRING": {
			_name;
		};

		default {
			str _name;
		};
	};

	if (_name isEqualTo "") then {
		_continue = false;
	}
	else {
		_khData = true;
	};
};

if !_continue exitWith {
	nil;
};

private _oldValue = if _khData then {
	[_namespace, _name, nil, false] call KH_fnc_getVariable;
}
else {
	_namespace getVariable _name;
};

if !(isNil "_oldValue") then {
	_value = switch _writeType do {
		case "OVERWRITE": {
			_value;
		};

		case "ADD": {
			_oldValue + _value;
		};

		case "SUBTRACT": {
			_oldValue - _value;
		};

		case "MULTIPLY": {
			_oldValue * _value;
		};

		case "DIVIDE": {
			_oldValue / _value;
		};

		case "RAISE": {
			_oldValue ^ _value;
		};

		case "REMAINDER": {
			_oldValue % _value;
		};

		case "INTERSECT": {
			_oldValue arrayIntersect _value;
		};

		case "APPEND": {
			_oldValue append _value;
			_oldValue;
		};

		case "PUSHBACK": {
			_oldValue pushBack _value;
			_oldValue;
		};

		case "PUSHBACK_UNIQUE": {
			_oldValue pushBackUnique _value;
			_oldValue;
		};

		case "SET": {
			_oldValue set [_value select 0, _value select 1, false];
			_oldValue;
		};

		case "SET_UNIQUE": {
			_oldValue set [_value select 0, _value select 1, true];
			_oldValue;
		};

		case "INSERT": {
			_oldValue insert _value;
			_oldValue;
		};

		case "INSERT_UNIQUE": {
			_oldValue insert [_value select 0, _value select 1, true];
			_oldValue;
		};

		case "MERGE_OVERWRITE": {
			_oldValue merge [_value, true];
			_oldValue;
		};

		case "MERGE": {
			_oldValue merge [_value, false];
			_oldValue;
		};
	};

	if ((_value isEqualType createHashMap) || (_value isEqualType [])) then {
		_continue = false;
	};
};

if ((_scope isEqualTo true) || (_scope isEqualTo "LOCAL")) exitWith {
	if _persistent then {
		[
			[_namespace, _name, _value, true, _writeType], 
			"KH_fnc_setVariable",
			true, 
			true, 
			if _persistent then {
				["PERSISTENT", [], {}, [_namespace, _name] joinString ""],
			}
			else {
				false;
			}
		] call KH_fnc_execute;
	}
	else {
		if _continue then {
			if !_khData then {
				_namespace setVariable [_name, _value];
			}
			else {				
				[_namespace, _name, _value, typeName _value, true] call KH_fnc_writeKhData;
			};
		};
	};

	["KH_eve_variableChanged", [_namespace, _name, _value, _oldValue, _writeType]] call CBA_fnc_localEvent;
};

[
	[_namespace, _name, _value, true, _writeType], 
	"KH_fnc_setVariable", 
	_scope, 
	true, 
	if _persistent then {
		["PERSISTENT", [], {}, [_namespace, _name] joinString ""],
	}
	else {
		false;
	}
] call KH_fnc_execute;

if !(isNil "_oldValue") then {
	_oldValue;
};