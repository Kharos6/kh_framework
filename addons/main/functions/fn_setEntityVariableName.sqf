params ["_entity", ["_name", ""]];
private _variableName = "";

if (_name == "") then {
	_variableName = [] call KH_fnc_generateUid;
}
else {
	_variableName = _name;
};

_entity setVehicleVarName _variableName;
missionNamespace setVariable [_variableName, _entity];

[
	[_entity, _variableName],
	{
		params ["_entity", "_variableName"];
		_entity setVehicleVarName _variableName;
		missionNamespace setVariable [_variableName, _entity, true];

		[
			[_entity, _variableName],
			{
				params ["_entity", "_variableName"];
				_entity setVehicleVarName _variableName;
			},
			["JIP", "GLOBAL", _entity, false, true, ""],
			"THIS_FRAME"
		] call KH_fnc_execute;
	},
	"SERVER",
	"THIS_FRAME"
] call KH_fnc_execute;

_variableName;