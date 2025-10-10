params [["_header", "", [""]], ["_message", [], [[]]], ["_throw", true, [true]], ["_verboseFormat", true, [true]], ["_sendToServer", false, [true]]];

if _throw then {
	throw ([_header, _message joinString ""] joinString "");
};

if !_verboseFormat then {
	_message insert [0, [[" | USER = ", profileName, " | "] joinString ""]];
	_message = _message joinString "";

	if !_sendToServer then {
		diag_log (text ([_header, _message] joinString ""));
	}
	else {
		if !isServer then {
			[
				[_header, _message],
				{
					params ["_header", "_message"];
					diag_log (text ([_header, _message] joinString ""));
				},
				"SERVER",
				true,
				false
			] call KH_fnc_execute;

			diag_log (text ([_header, _message] joinString ""));
		}
		else {
			diag_log (text ([_header, _message] joinString ""));
		};
	};
}
else {
	_message insert [0, [["USER = ", profileName] joinString ""]];

	if !_sendToServer then {
		diag_log (text _header);
		
		{
			diag_log (text (["   ", _x] joinString ""));
		} forEach _message;
	}
	else {
		if !isServer then {
			[
				[_header, _message],
				{
					params ["_header", "_message"];
					diag_log (text _header);
					
					{
						diag_log (text (["   ", _x] joinString ""));
					} forEach _message;
				},
				"SERVER",
				true,
				false
			] call KH_fnc_execute;

			diag_log (text _header);
			
			{
				diag_log (text (["   ", _x] joinString ""));
			} forEach _message;
		}
		else {
			diag_log (text _header);
			
			{
				diag_log (text (["   ", _x] joinString ""));
			} forEach _message;
		};
	};
};