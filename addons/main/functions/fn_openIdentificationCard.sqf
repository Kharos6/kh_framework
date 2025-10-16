params [
	["_name", "", [""]], 
	["_gender", "", [""]], 
	["_race", "", [""]], 
	["_dateOfBirth", "", [""]], 
	["_profession", "", [""]], 
	["_ethnos", "", [""]], 
	["_cardNumber", "", [""]], 
	["_dateOfIssue", "", [""]], 
	["_dateOfExpiry", "", [""]]
];

private _information = [
	["NAME: ", _name] joinString "",
	["GENDER: ", _gender] joinString "",
	["RACE: ", _race] joinString "",
	["DATE OF BIRTH: ", _dateOfBirth] joinString "",
	["PROFESSION: ", _profession] joinString "",
	["ETHNOS: ", _ethnos] joinString "",
	["CARD NUMBER: ", _cardNumber] joinString "",
	["DATE OF ISSUE: ", _dateOfIssue] joinString "",
	["DATE OF EXPIRY: ", _dateOfExpiry] joinString ""
];

private _display = createDialog ["KH_ResourceIdentificationCard", true];
private _count = (count _information) - 1;
private _idcOffset = 101;

for "_i" from 0 to _count do {
	ctrlSetText [_idcOffset, _information select _i];
	_idcOffset = _idcOffset + 1;
};

_display;