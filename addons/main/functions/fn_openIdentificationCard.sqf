params ["_name", "_gender", "_race", "_dateOfBirth", "_profession", "_nationality", "_cardNumber", "_dateOfIssue", "_dateOfExpiry"];

private _information = [
	format ["NAME: %1", _name],
	format ["GENDER: %1", _gender],
	format ["RACE: %1", _race],
	format ["DATE OF BIRTH: %1", _dateOfBirth],
	format ["PROFESSION: %1", _profession],
	format ["NATIONALITY: %1", _nationality],
	format ["CARD NUMBER: %1", _cardNumber],
	format ["DATE OF ISSUE: %1", _dateOfIssue],
	format ["DATE OF EXPIRY: %1", _dateOfExpiry]
];

private _display = createDialog ["KH_ResourceIdentificationCard", true];
private _count = (count _information) - 1;
private _idcOffset = 101;
private _i = 0;

for "_i" from 0 to _count do {
	ctrlSetText [_idcOffset, _information select _i];
	_idcOffset = _idcOffset + 1;
};

true;