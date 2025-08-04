params [["_formula", "", [""]]];

if (_formula isEqualTo "") exitWith {
	nil;
};

("kh_framework" callExtension ["MathOperation", _this]) params ["_result", "_returnCode"];

if ([_returnCode] call KH_fnc_parseBoolean) exitWith {
	diag_log (text ([_result, " | EXTENSION = kh_framework | FUNCTION = MathOperation | ARGUMENTS = ", _this] joinString ""));
	nil;
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