params ["_value"];

if (isNil "_value") exitWith {
	false;
};

switch _value do {
	case false: {
		false;
	};

	case 0: {
		false;
	};

	case "FALSE": {
		false;
	};

	case "false": {
		false;
	};

	case "0": {
		false;
	};

	default {
		true;
	};
};