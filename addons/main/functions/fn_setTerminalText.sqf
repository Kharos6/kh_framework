params ["_identifierOutput", "_text", ["_overwrite", false]];
private _output = "";

if _overwrite then {
	missionNamespace setVariable [_identifierOutput, _text, true];
	_output = missionNamespace getVariable [_identifierOutput, ""];
}
else {
	_output = [missionNamespace getVariable [_identifierOutput, ""], _text] joinString "";
	missionNamespace setVariable [_identifierOutput, _output, true];
};

_output;