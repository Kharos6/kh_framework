params [["_entity", objNull, [objNull, teamMemberNull, grpNull]], ["_name", "", [""]]];

if (_entity isEqualType teamMemberNull) then {
	_entity = agent _entity;
};

if (_entity isEqualType objNull) then {
	if (_name isEqualTo "") then {
		private _variableName = vehicleVarName _entity;

		if (_variableName isEqualTo "") then {
			_name = hashValue _entity;
			_entity setVariable ["KH_var_generatedVariableName", true, true];
		}
		else {
			_name = _variableName;
		};
	}
	else {
		_entity setVariable ["KH_var_generatedVariableName", false, true];
	};

	missionNamespace setVariable [_name, _entity, true];
	_entity setVehicleVarName _name;

	[
		[_entity, _name],
		{
			params ["_entity", "_name"];
			_entity setVehicleVarName _name;
		},
		"GLOBAL",
		true,
		true
	] call KH_fnc_execute;
}
else {
	if (_name isEqualTo "") then {
		_name = hashValue _entity;
		_entity setVariable ["KH_var_generatedVariableName", true, true];
	}
	else {
		_entity setVariable ["KH_var_generatedVariableName", false, true];
	};

	missionNamespace setVariable [_name, _entity, true];
};

_name;