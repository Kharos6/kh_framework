params [["_identifierOutput", "", [""]], ["_text", "", [""]], ["_overwrite", false, [true]]];

if _overwrite then {
	missionNamespace setVariable [_identifierOutput, _text, true];
	missionNamespace getVariable [_identifierOutput, ""];
}
else {
	private _output = [missionNamespace getVariable [_identifierOutput, ""], _text] joinString "";
	missionNamespace setVariable [_identifierOutput, _output, true];
	_output;
};