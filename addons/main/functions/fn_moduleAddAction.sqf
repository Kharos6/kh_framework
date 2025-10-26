KH_var_postInitExecutions pushBack [
    _this,
    {
        params [["_logic", objNull, [objNull]], ["_units", [], [[]]], ["_activated", true, [true]]];

        if _activated then {
            {
                [
                    _x,
                    _logic getVariable ["KH_ModuleAddActionName", ""],
                    [],
                    [
                        compile (_logic getVariable ["KH_ModuleAddActionFunctionStart", ""]),
                        compile (_logic getVariable ["KH_ModuleAddActionFunctionProgress", ""]),
                        compile (_logic getVariable ["KH_ModuleAddActionFunctionInterrupt", ""]),
                        compile (_logic getVariable ["KH_ModuleAddActionFunctionComplete", ""])
                    ],
                    [
                        compile (_logic getVariable ["KH_ModuleAddActionConditionShow", "true"]),
                        compile (_logic getVariable ["KH_ModuleAddActionConditionProgress", "true"]),
                        compile (_logic getVariable ["KH_ModuleAddActionConditionComplete", "true"])
                    ],
                    _logic getVariable ["KH_ModuleAddActionRepeatable", false],
                    _logic getVariable ["KH_ModuleAddActionExclusive", true],
                    "PLAYERS",
                    _logic getVariable ["KH_ModuleAddActionJIP", true],
                    parseNumber (_logic getVariable ["KH_ModuleAddActionDuration", "0"]),
                    parseNumber (_logic getVariable ["KH_ModuleAddActionDistance", "3"]),
                    _logic getVariable ["KH_ModuleAddActionShowImmediately", true],
                    _logic getVariable ["KH_ModuleAddActionShowUnconscious", false],
                    _logic getVariable ["KH_ModuleAddActionHideOnUse", false],
                    _logic getVariable ["KH_ModuleAddActionDrawHint", false],
                    [_logic getVariable ["KH_ModuleAddActionDetectionShow", true], _logic getVariable ["KH_ModuleAddActionDetectionProgress", false]],
                    _logic getVariable ["KH_ModuleAddActionUserInput", ""],
                    _logic getVariable ["KH_ModuleAddActionProgressDisplay", "BAR"],
                    _logic getVariable ["KH_ModuleAddActionIdentifier", ""],
                    if ((_logic getVariable ["KH_ModuleAddActionParentUnit", ""]) isEqualTo "") then {
                        true;
                    }
                    else {
                        missionNamespace getVariable [_logic getVariable ["KH_ModuleAddActionParentUnit", ""], objNull];
                    }
                ] call KH_fnc_addAction;
            } forEach _units;
        };
    }
];

nil;