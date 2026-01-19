isNil {
    KH_var_postInitExecutions pushBack [
        _this,
        {
            params [["_logic", objNull, [objNull]], ["_units", [], [[]]], ["_activated", true, [true]]];

            if _activated then {
                [
                    [
                        _logic getVariable ["KH_ModuleInitializeTTSModel", ""],
                        parseNumber (_logic getVariable ["KH_ModuleInitializeTTSCPUThreads", "2"]),
                        parseNumber (_logic getVariable ["KH_ModuleInitializeTTSNoiseScale", "0.667"]),
                        parseNumber (_logic getVariable ["KH_ModuleInitializeTTSNoiseScaleW", "0.8"]),
                        parseNumber (_logic getVariable ["KH_ModuleInitializeTTSLengthScale", "1"]),
                        compile (_logic getVariable ["KH_ModuleInitializeTTSGeneratedFunction", ""]),
                        compile (_logic getVariable ["KH_ModuleInitializeTTSFinishedFunction", ""])
                    ],
                    "KH_fnc_initializeTts",
                    "PLAYERS",
                    true,
                    true
                ] call KH_fnc_execute;
            };
        }
    ];
};

nil;