KH_var_postInitExecutions pushBack [
    _this,
    {
        params [["_logic", objNull, [objNull]], ["_units", [], [[]]], ["_activated", true, [true]]];

        if _activated then {
            [
                [
                    _logic getVariable ["KH_ModuleInitializeSTTModel", ""],
                    parseNumber (_logic getVariable ["KH_ModuleInitializeSTTCPUThreads", "2"]),
                    compile (_logic getVariable ["KH_ModuleInitializeSTTTranscriptionFunction", ""])
                ],
                "KH_fnc_initializeStt",
                "PLAYERS",
                true,
                true
            ] call KH_fnc_execute;
        };
    }
];

nil;