params [
	["_namespace", missionNamespace, ["", objNull, teamMemberNull, grpNull, locationNull, missionNamespace, taskNull, controlNull, displayNull]], 
	"_name",
	["_type", "", [""]],
	"_value",
	["_target", true, [true, 0, "", [], {}, objNull, teamMemberNull, grpNull, sideUnknown, locationNull]], 
	["_writeType", true, [true, ""]],
	["_special", false, [true, ""]]
];

if (_type isEqualTo "") then {
	_type = typeName _value;
};

private _namespaceType = "STANDARD";

if (_namespace isEqualType "") then {
	_name = (["", _name] call KH_fnc_serializeValue) param [1];
	_namespaceType = ["LUA", "KH_DATA"] select (_namespace isEqualTo "KH_DATA");
}
else {
	if (_namespace isEqualTo missionProfileNamespace) then {		
		saveMissionProfileNamespace;
	};
};

private "_oldValueType";
private "_oldValue";

switch _namespaceType do {
	case "STANDARD": {
		_oldValue = _namespace getVariable _name;

		_oldValueType = if !(isNil "_oldValue") then {
			typeName _oldValue;
		}
		else {
			_type;
		};
	};

	case "KH_DATA": {
		private _currentVariable = [_namespace, _name] call KH_fnc_readKhData;
		_oldValueType = _currentVariable param [0];
		_oldValue = _currentVariable call KH_fnc_parseValue;
	};

	case "LUA": {
		private _currentVariable = [_name] call KH_fnc_luaGetVariable;
		_oldValueType = _currentVariable param [0];
		_oldValue = _currentVariable call KH_fnc_parseValue;
	};
};

private _continue = true;

if (!(isNil "_oldValue") && !(isNil "_value")) then {
	if (_type isNotEqualTo _oldValueType) then {
		_writeType = true;
	};

	_value = switch _writeType do {
		case true;
		case "OVERWRITE": {
			if (_oldValue isEqualTypeAny [[], createHashMap]) then {
				if (_oldValue isEqualType []) then {
					_oldValue resize 0;
					_oldValue append _value;
				}
				else {
					private _keys = [];

					{
						_keys insert _x;
					} forEach (keys _oldValue);

					{
						_oldValue deleteAt _x;
					} forEach _keys;

					_oldValue insert (toArray _value);
				};

				_oldValue;
			}
			else {
				_value;
			};
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
			private _intersectedArray = _oldValue arrayIntersect _value;
			_oldValue resize 0;
			_oldValue append _intersectedArray;
			_oldValue;
		};

		case "DEDUPLICATE": {
			private _cleanArray = [];

			{
				_cleanArray pushBackUnique _x;
			} forEach _oldValue;

			_oldValue resize 0;
			_oldValue append _cleanArray;
			_oldValue;
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

		case "MERGE": {
			_oldValue merge [_value, true];
			_oldValue;
		};

		case "MERGE_UNIQUE": {
			_oldValue merge [_value, false];
			_oldValue;
		};

		default {
			_oldValue;
		};
	};

	if ((_namespaceType isEqualTo "STANDARD") && (_value isEqualTypeAny [[], createHashMap])) then {
		_continue = false;
	};
};

if ((_target isEqualTo true) || (_target isEqualTo "LOCAL")) then {
	if (_special isNotEqualTo false) then {
		if (_special isEqualTo true) then {
			_special = "JIP";
		};

		[
			[_namespace, _name, _type, _value, true, true, false], 
			"KH_fnc_setVariable",
			true, 
			true,
			if (_special isEqualTo "JIP") then {
				["JIP", true, false, ""];
			}
			else {
				["PERSISTENT", true, [], {}, [_namespace, _name] joinString ""];
			}
		] call KH_fnc_execute;
	}
	else {
		if _continue then {
			switch _namespaceType do {
				case "STANDARD": {
					_namespace setVariable [_name, _value];
				};

				case "KH_DATA": {
					([_type, _value] call KH_fnc_serializeValue) params ["_type", "_value"];
					[_namespace, _name, _type, _value, true] call KH_fnc_writeKhData;
				};

				case "LUA": {
					([_type, _value] call KH_fnc_serializeValue) params ["_type", "_value"];
					[_name, _type, _value] call KH_fnc_luaSetVariable;
				};
			};
		};
	};

	["KH_eve_variableChanged", [_namespace, _name, _value, _oldValue, _writeType]] call CBA_fnc_localEvent;
}
else {
	[
		[_namespace, _name, _type, _value, true, true, false], 
		"KH_fnc_setVariable", 
		_target, 
		true,
		if (_special isNotEqualTo false) then {
			if (_special isEqualTo "JIP") then {
				["JIP", true, false, ""];
			}
			else {
				["PERSISTENT", true, [], {}, [_namespace, _name] joinString ""];
			};
		}
		else {
			false;
		}
	] call KH_fnc_execute;
};

_oldValue;