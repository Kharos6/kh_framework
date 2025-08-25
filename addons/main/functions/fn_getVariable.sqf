params [
	["_namespace", missionNamespace, ["", missionNamespace, objNull, teamMemberNull, grpNull, locationNull, taskNull, controlNull, displayNull]], 
	"_name", 
	"_defaultValue", 
	["_target", false, [true, []]]
];

if (_target isEqualTo false) exitWith {
	if (_namespace isEqualType "") then {
		_name = switch (typeName _name) do {
			case "OBJECT": {
				if (isPlayer _name) then {
					getPlayerUID _name;
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

		if (_name isNotEqualTo "") then {
			[_namespace, _name, _defaultValue] call KH_fnc_readKhData;
		}
		else {
			_defaultValue;
		};
	}
	else {
		_namespace getVariable [_name, _defaultValue];
	};
};

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
		_callbackTarget,
		[_namespace, _name, _defaultValue],
		{
			params ["_namespace", "_name", "_defaultValue"];
			[[_namespace, _name, _defaultValue, false] call KH_fnc_getVariable];
		}
	]
] call KH_fnc_execute;