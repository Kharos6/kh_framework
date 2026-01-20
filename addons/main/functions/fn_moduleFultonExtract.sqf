isNil {
    KH_var_postInitExecutions pushBack [
        _this,
        {
            params [["_logic", objNull, [objNull]], ["_units", [], [[]]], ["_activated", true, [true]]];

            if _activated then {
                {
                    [
                        _x,
                        (parseSimpleArray (["[", _logic getVariable ["KH_ModuleFultonExtractVehicles", ""], "]"] joinString "")) apply {missionNamespace getVariable [_x, objNull];},
                        _logic getVariable ["KH_ModuleFultonExtractHeight", 100],
                        parseNumber (_logic getVariable ["KH_ModuleFultonExtractDistance", "10"]),
                        parseNumber (_logic getVariable ["KH_ModuleFultonExtractMaximumParticipants", "10"]),
                        parseNumber (_logic getVariable ["KH_ModuleFultonExtractDuration", "15"]),
                        _logic getVariable ["KH_ModuleFultonExtractName", ""]
                    ] call KH_fnc_fultonExtract;
                } forEach _units;
            };
        }
    ];
};

nil;