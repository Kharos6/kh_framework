params ["_namespace", "_name", ["_defaultValue", nil], ["_target", []]];

if (_target isEqualTo []) exitWith {
	switch (typeName _namespace) do {
		case "ARRAY": {
			private _values = [];
			private _i = 0;

			for "_i" from 0 to ((count _namespace) - 1) do {
				_values pushBack ([_namespace select _i, _name select _i, _defaultValue select _i, []] call KH_fnc_getVariable);
			};

			_values;
		};

		case "STRING": {
			private _parsedName = switch (typeName _name) do {
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

			if (_parsedName isNotEqualTo "") then {
				[_namespace, _parsedName, _defaultValue] call KH_fnc_readKhData;
			}
			else {
				_defaultValue;
			};
		};

		default {
			_namespace getVariable [_name, _defaultValue];
		};
	};
};

_target params ["_targetMachine", "_arguments", "_function"];

[
	[_arguments, _function], 
	{
		params ["_args", "_function"];
		_argsCallback params ["_value"];
		call _function;
	},
	[
		"CALLBACK",
		_targetMachine,
		[_namespace, _name, _defaultValue],
		{
			params ["_namespace", "_name", "_defaultValue"];
			[[_namespace, _name, _defaultValue, []] call KH_fnc_getVariable];
		}
	],
	true
] call KH_fnc_execute;

true;