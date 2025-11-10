KH_var_postInitExecutions pushBack [
    _this,
    {
        params [["_logic", objNull, [objNull]], ["_units", [], [[]]], ["_activated", true, [true]]];

        if _activated then {
            private _visualTriggers = [];

            {
                _visualTriggers pushBack ([
                    _x,
                    parseNumber (_logic getVariable ["KH_ModuleVisualTriggerScreenPercentage", "0.5"]),
                    parseNumber (_logic getVariable ["KH_ModuleVisualTriggerMinimumDistance", "1"]),
                    parseNumber (_logic getVariable ["KH_ModuleVisualTriggerMaximumDistance", "1000"]),
                    compile (_logic getVariable ["KH_ModuleVisualTriggerConditionServer", "true"]),
                    compile (_logic getVariable ["KH_ModuleVisualTriggerConditionPlayer", "true"]),
                    compile (_logic getVariable ["KH_ModuleVisualTriggerTrueFunctionServer", ""]),
                    compile (_logic getVariable ["KH_ModuleVisualTriggerFalseFunctionServer", ""]),
                    compile (_logic getVariable ["KH_ModuleVisualTriggerTrueFunctionPlayer", ""]),
                    compile (_logic getVariable ["KH_ModuleVisualTriggerFalseFunctionPlayer", ""]),
                    _logic getVariable ["KH_ModuleVisualTriggerRepeatable", false],
                    parseNumber (_logic getVariable ["KH_ModuleVisualTriggerInterval", "0.5"]),
                    _logic getVariable ["KH_ModuleVisualTriggerShared", true]
                ] call KH_fnc_visualTrigger);
            } forEach _units;

            [
                ["ENTITY", _logic, "REMOTE"],
                "Deleted",
                [_visualTriggers],
                {
                    _args params ["_visualTriggers"];
                    
                    {
                        [_x] call KH_fnc_removeHandler;
                    } forEach _visualTriggers;
                }
            ] call KH_fnc_addEventHandler;
        };
    }
];

nil;