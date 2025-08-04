params [["_header", "WARNING: ", [""]], ["_message", [], [[]]], ["_simpleFormat", false, [true]]];

if _simpleFormat then {
	diag_log (text ([_header, _message joinString ""] joinString ""));
}
else {
	diag_log (text _header);
	
	{
		diag_log (text (["   ", _x] joinString ""));
	} forEach _message;
};

nil;