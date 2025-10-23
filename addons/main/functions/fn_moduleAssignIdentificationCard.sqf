isNil {
	params [["_logic", objNull, [objNull]], ["_units", [], [[]]], ["_activated", true, [true]]];

	if _activated then {
        [
            [_units],
            {
                params ["_units"];

                {
                    [
                        _x,
                        _logic getVariable ["KH_ModuleAssignIdentificationCardName", ""],
                        _logic getVariable ["KH_ModuleAssignIdentificationCardGender", ""],
                        _logic getVariable ["KH_ModuleAssignIdentificationCardRace", ""],
                        _logic getVariable ["KH_ModuleAssignIdentificationCardDateOfBirth", ""],
                        _logic getVariable ["KH_ModuleAssignIdentificationCardProfession", ""],
                        _logic getVariable ["KH_ModuleAssignIdentificationCardEthnos", ""],
                        _logic getVariable ["KH_ModuleAssignIdentificationCardCardNumber", ""],
                        _logic getVariable ["KH_ModuleAssignIdentificationCardDateOfIssue", ""],
                        _logic getVariable ["KH_ModuleAssignIdentificationCardDateOfExpiry", ""]
                    ] call KH_fnc_assignIdentificationCard;
                } forEach _units;
            },
            "SERVER",
            true,
            false
        ] call KH_fnc_execute;
	};
};

nil;