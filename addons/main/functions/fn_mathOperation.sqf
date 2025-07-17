params [["_formula", "", [""]]];
private _result = if ((_file isEqualTo "") || (_variable isEqualTo "")) then {
	false;
}
else {
	("kh_framework" callExtension ["MathOperation", _this]) select 0;
};

if ("KH_ERROR: " in _result) exitWith {
	false;
};

switch _result do {
	case "true": {
		true;
	};

	case "false": {
		false;
	};

	default {
		parseNumber _result;
	};
};