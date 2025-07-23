params [["_value", false, ["", 0, true]]];

switch _value do {
	case true: {
		true;
	};

	case false: {
		false;
	};
	
	case "TRUE": {
		true;
	};

	case "FALSE": {
		false;
	};

	case "True": {
		true;
	};

	case "False": {
		false;
	};

	case "true": {
		true;
	};

	case "false": {
		false;
	};

	case 0: {
		false;
	};

	case 1: {
		true;
	};

	case "0": {
		false;
	};

	case "1": {
		true;
	};

	default {
		false;
	};
};