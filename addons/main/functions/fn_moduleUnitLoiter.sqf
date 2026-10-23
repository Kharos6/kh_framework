isNil {
    KH_var_postInitExecutions pushBack [
        _this,
        {
            params [["_logic", objNull, [objNull]], ["_units", [], [[]]], ["_activated", true, [true]]];
            
            {
                [
                    _x, 
                    _activated, 
                    parseNumber (_logic getVariable ["KH_ModuleUnitLoiterRadius", "100"]),
                    _logic getVariable ["KH_ModuleUnitLoiterEnterBuildings", true],
                    [
                        parseNumber (_logic getVariable ["KH_ModuleUnitLoiterRandomDelayMinimum", "5"]),
                        parseNumber (_logic getVariable ["KH_ModuleUnitLoiterRandomDelayMiddle", "15"]),
                        parseNumber (_logic getVariable ["KH_ModuleUnitLoiterRandomDelayMaximum", "30"])
                    ]
                ] call KH_fnc_setUnitLoiter;
            } forEach _units;
        }
    ];
};

nil;