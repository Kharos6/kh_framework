params [
	["_namespace", missionNamespace, ["", missionNamespace, objNull, teamMemberNull, grpNull, locationNull, taskNull, controlNull, displayNull]], 
	"_name", 
	"_defaultValue", 
	["_target", [], [[]]]
];

if (_target isEqualTo []) then {
	if (_namespace isEqualType "") then {
		_name = (["", _name] call KH_fnc_serializeValue) param [1];

		private _value = if (_namespace isEqualTo "LUA") then {
			([_name] call KH_fnc_luaGetVariable) call KH_fnc_parseValue;
		}
		else {
			([_namespace, _name] call KH_fnc_readKhData) call KH_fnc_parseValue;
		};

		[_value, _defaultValue] select (isNil "_value");
	}
	else {
		_namespace getVariable [_name, _defaultValue];
	};
}
else {
	_target params [
		["_callbackTarget", true, [true, 0, "", [], {}, objNull, teamMemberNull, grpNull, sideUnknown, locationNull]],
		"_arguments", 
		["_function", {}, ["", {}]]
	];

	[
		_arguments,
		_function,
		_callbackTarget,
		true,
		[
			"CALLBACK",
			[_namespace, _name, _defaultValue],
			{
				params ["_namespace", "_name", "_defaultValue"];
				[[_namespace, _name, _defaultValue, []] call KH_fnc_getVariable];
			}
		]
	] call KH_fnc_execute;
};