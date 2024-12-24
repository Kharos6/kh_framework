params ["_unit", "_name", "_gender", "_race", "_dateOfBirth", "_profession", "_nationality", "_cardNumber", "_dateOfIssue", "_dateOfExpiry"];

if (_name == "") then {
	_name = name _unit;
};

_unit setVariable ["KH_var_identificationCardInformation", [_name, _gender, _race, _dateOfBirth, _profession, _nationality, _cardNumber, _dateOfIssue, _dateOfExpiry], true];

[
	[_unit],
	{
		params ["_unit"];

		if (_unit != player) then {
			_unit addAction [
				"View Identification Card",
				{
					params ["_target"];
					(_target getVariable ["KH_var_identificationCardInformation", []]) call KH_fnc_openIdentificationCard;
				},
				nil,
				1.5,
				false,
				true,
				"",
				"true",
				4,
				false,
				"",
				""
			];
		}
		else {
			_unit addAction [
				"View Own Identification Card",
				{
					params ["_target"];
					(_target getVariable ["KH_var_identificationCardInformation", []]) call KH_fnc_openIdentificationCard;
				},
				nil,
				1.5,
				false,
				true,
				"",
				"true",
				4,
				false,
				"",
				""
			];
		};
	},
	["JIP", "PLAYERS", _unit, true, false], 
	"THIS_FRAME"
] call KH_fnc_execute;

true;