isNil {
    params [["_mode", "", [""]], ["_input", [], [[]]]];

    if (isNil "KH_var_allModifiedTerrain") then {
        KH_var_allModifiedTerrain = createHashMap;
    };

    if (isNil "KH_var_allTerrainModifierModules") then {
        KH_var_allTerrainModifierModules = [];
    };

    if !((_input select 0) in KH_var_allTerrainModifierModules) then {
        KH_var_allTerrainModifierModules pushBack (_input select 0);
    };

    switch _mode do {
        case "init": {
            _input params [["_logic", objNull, [objNull]], ["_activated", true, [true]], ["_curatorPlaced", false, [true]]];
            if is3DEN exitWith {};

            if _activated then {
                private _area = _logic getVariable "objectarea";

                private _modifiedTerrain = [
                    getPosASL _logic,
                    [_area select 0, _area select 1],
                    _area select 3,
                    getDir _logic,
                    parseSimpleArray (["[", _logic getVariable ["KH_ModuleEditTerrainHeightCurve", ""], "]"] joinString ""),
                    _logic getVariable ["KH_ModuleEditTerrainHeightApplicationType", "ADD"],
                    _logic getVariable ["KH_ModuleEditTerrainHeightCurveType", "LINEAR"],
                    [parseNumber (_logic getVariable ["KH_ModuleEditTerrainHeightChangeDuration", "0"]), _logic getVariable ["KH_ModuleEditTerrainHeightAnimationCurveType", "LINEAR"]],
                    _logic getVariable ["KH_ModuleEditTerrainHeightAdjustObjectHeight", true]
                ] call KH_fnc_editTerrainHeight;
            };
        };

        case "unregisteredFromWorld3DEN": {
            _input params [["_logic", objNull, [objNull]]];
            private _logicListing = KH_var_allTerrainModifierModules find _logic;

            if (_logicListing isNotEqualTo -1) then {
                KH_var_allTerrainModifierModules deleteAt _logicListing;
            };

            {
                setTerrainHeight [[[_x select 0, _x select 1, _y]], true];
            } forEach KH_var_allModifiedTerrain;

            KH_var_allModifiedTerrain = createHashMap;

            {
                if (_x isNotEqualTo _logic) then {
                    private _area = (_x get3DENAttribute "size3") select 0;

                    private _modifiedTerrain = [
                        getPosASL _x,
                        [_area select 0, _area select 1],
                        (_x get3DENAttribute "isRectangle") select 0,
                        getDir _x,
                        parseSimpleArray (["[", (_x get3DENAttribute "KH_ModuleEditTerrainHeightCurve") select 0, "]"] joinString ""),
                        (_x get3DENAttribute "KH_ModuleEditTerrainHeightApplicationType") select 0,
                        (_x get3DENAttribute "KH_ModuleEditTerrainHeightCurveType") select 0,
                        0,
                        (_x get3DENAttribute "KH_ModuleEditTerrainHeightAdjustObjectHeight") select 0
                    ] call KH_fnc_editTerrainHeight;
                };
            } forEach KH_var_allTerrainModifierModules;
        };

        default {
            _input params [["_logic", objNull, [objNull]]];

            {
                setTerrainHeight [[[_x select 0, _x select 1, _y]], true];
            } forEach KH_var_allModifiedTerrain;

            KH_var_allModifiedTerrain = createHashMap;
            
            {
                if ((_x get3DENAttribute "KH_ModuleEditTerrainHeightShowChanges") select 0) then {
                    private _area = (_x get3DENAttribute "size3") select 0;

                    private _modifiedTerrain = [
                        getPosASL _x,
                        [_area select 0, _area select 1],
                        (_x get3DENAttribute "isRectangle") select 0,
                        getDir _x,
                        parseSimpleArray (["[", (_x get3DENAttribute "KH_ModuleEditTerrainHeightCurve") select 0, "]"] joinString ""),
                        (_x get3DENAttribute "KH_ModuleEditTerrainHeightApplicationType") select 0,
                        (_x get3DENAttribute "KH_ModuleEditTerrainHeightCurveType") select 0,
                        0,
                        (_x get3DENAttribute "KH_ModuleEditTerrainHeightAdjustObjectHeight") select 0
                    ] call KH_fnc_editTerrainHeight;
                };
            } forEach KH_var_allTerrainModifierModules;
        };
    };
};

nil;