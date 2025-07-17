params [["_file", "", [""]], ["_binarize", true, [true]], ["_wipe", false, [true]]];

if (_file isEqualTo "") exitWith {
	false;
};

private _result = "kh_framework" callExtension ["ManageKHData", _this];

if ("KH_ERROR: " in _result) then {
	false;
}
else {
	true;
};