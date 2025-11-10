KH_var_postInitExecutions pushBack [
    _this,
    {
        params [["_logic", objNull, [objNull]], ["_units", [], [[]]], ["_activated", true, [true]]];

        if _activated then {
            {
                [
                    objNull,
                    _x,
                    _logic getVariable ["KH_ModuleEquipableObjectBone", ""],
                    _logic getVariable ["KH_ModuleEquipableObjectPosition", [0, 0, 0]],
                    _logic getVariable ["KH_ModuleEquipableObjectRotation", [0, 0, 0]],
                    parseNumber (_logic getVariable ["KH_ModuleEquipableObjectScale", "1"]),
                    _logic getVariable ["KH_ModuleEquipableObjectDisableCollision", true],
                    _logic getVariable ["KH_ModuleEquipableObjectHideInVehicles", true],
                    _logic getVariable ["KH_ModuleEquipableObjectExclusive", true],
                    _logic getVariable ["KH_ModuleEquipableObjectName", ""]
                ] call KH_fnc_equipableObject;
            } forEach _units;
        };
    }
];

nil;