params ["_name", "_gender", "_dateOfBirth", "_profession", "_nationality", "_cardNumber", "_dateOfIssue", "_dateOfExpiry"];

private _information = [
	format ["Name: %1", _name],
	format ["Gender: %1", _gender],
	format ["Date Of Birth: %1", _dateOfBirth],
	format ["Profession: %1", _profession],
	format ["Nationality: %1", _nationality],
	format ["Card Number: %1", _cardNumber],
	format ["Date Of Issue: %1", _dateOfIssue],
	format ["Date Of Expiry: %1", _dateOfExpiry]
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