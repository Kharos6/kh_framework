isNil {
	params [["_logic", objNull, [objNull]], ["_units", [], [[]]], ["_activated", true, [true]]];

	if _activated then {
        private _vehicles = [];

        {
            _vehicles pushBack (missionNamespace getVariable [_x, objNull]);
        } forEach (parseSimpleArray (["[", _logic getVariable ["KH_ModuleFultonExtractVehicles", ""], "]"] joinString ""));

        {
            [
                _x,
                _vehicles,
                parseNumber (_logic getVariable ["KH_ModuleFultonExtractHeight", "100"]),
                parseNumber (_logic getVariable ["KH_ModuleFultonExtractDistance", "10"]),
                parseNumber (_logic getVariable ["KH_ModuleFultonExtractMaximumParticipants", "10"]),
                parseNumber (_logic getVariable ["KH_ModuleFultonExtractDuration", "15"]),
                _logic getVariable ["KH_ModuleFultonExtractName", ""]
            ] call KH_fnc_fultonExtract;
        } forEach _units;
	};
};

nil;