private _name = ["KH_var_collectedData_", CBA_missionTime] joinString "";
missionNamespace setVariable [_name, createHashMap];
private _data = missionNamespace getVariable _name;

if ((missionNamespace getVariable ["KH_var_dataCollectionHandlers", []]) isNotEqualTo []) then {
    [missionNamespace getVariable ["KH_var_dataCollectionHandlers", []]] call KH_fnc_removeHandler;
    missionNamespace setVariable ["KH_var_dataCollectionHandlers", []];
};

if (missionNamespace isNil "KH_var_allCollectedData") then {
    missionNamespace setVariable ["KH_var_allCollectedData", []];
};

(missionNamespace getVariable ["KH_var_allCollectedData", []]) pushBack [CBA_missionTime, _data];
_data set ["BuildingChanged", []];
_data set ["Environment", []];

_data set [
    "Header", 
    [
        briefingName,
        missionName,
        call KH_fnc_getMissionAttributes
    ]
];

_data set ["Roads", ([worldSize / 2, worldSize / 2] nearRoads (worldSize * sqrt 2 / 2)) apply {getPosATLVisual _x;}];
_data set ["SideRelations", []];

_data set ["TerrainObjects", (nearestTerrainObjects [[worldSize / 2, worldSize / 2], ["Thing", "Static"], worldSize * sqrt 2 / 2, false, true]) apply {
    [typeOf _x, getPosATLVisual _x, boundingBoxReal [_x, "FireGeometry"]];
}];

private _result = [
    missionNamespace getVariable ["KH_var_allCollectedData", []],
    _data,
    [
        [
            "MISSION",
            "BuildingChanged",
            [_data],
            {
                params ["_from", "_to", "_isRuin"];
                _args params ["_data"];
                (_data get "BuildingChanged") pushBack [CBA_missionTime, [typeOf _from, getPosATLVisual _from, netId _to, _isRuin]];
            }
        ] call KH_fnc_addEventHandler,
        [
            [_data],
            {
                params ["_data"];
                private _missionData = (call KH_fnc_getMissionAttributes) select 1;

                if ((missionNamespace getVariable ["KH_var_collectedEnvironment", []]) isNotEqualTo (_missionData select [0, 13])) then {
                    (_data get "Environment") pushBack [CBA_missionTime, _missionData select [0, 13]];
                    missionNamespace setVariable ["KH_var_collectedEnvironment", _missionData select [0, 13]];
                };

                if ((missionNamespace getVariable ["KH_var_collectedSideRelations", []]) isNotEqualTo (_missionData select 13)) then {
                    (_data get "SideRelations") pushBack [CBA_missionTime, _missionData select 13];
                    missionNamespace setVariable ["KH_var_collectedSideRelations", _missionData select 13];
                };
            },
            true,
            0.1,
            false
        ] call KH_fnc_execute
    ]
];

missionNamespace setVariable ["KH_var_dataCollectionHandlers", _result select 2];
_result;