isNil {
    KH_var_postInitExecutions pushBack [
        _this,
        {
            params [["_logic", objNull, [objNull]], ["_units", [], [[]]], ["_activated", true, [true]]];

            if _activated then {
                {
                    [
                        _x,
                        _logic getVariable ["KH_ModuleAssignTerminalName", ""],
                        _logic getVariable ["KH_ModuleAssignTerminalIdentifier", ""],
                        _logic getVariable ["KH_ModuleAssignTerminalDescription", ""],
                        compile (_logic getVariable ["KH_ModuleAssignTerminalCondition", ""]),
                        compile (_logic getVariable ["KH_ModuleAssignTerminalFunction", ""])
                    ] call KH_fnc_assignTerminal;
                } forEach _units;
            };
        }
    ];
};

nil;