isNil {
    params [["_mode", "", [""]], ["_input", [], [[]]]];

    if (isNil "KH_var_allModulePostProcessing") then {
        KH_var_allModulePostProcessing = createHashMap;
    };

    switch _mode do {
        case "init": {
            _input params [["_logic", objNull, [objNull]], ["_activated", true, [true]], ["_curatorPlaced", false, [true]]];
            if is3DEN exitWith {};
            private _hash = hashValue _logic;
            
            if !(KH_var_allModulePostProcessing isNil _hash) then {
                removeRenderHandler (KH_var_allModulePostProcessing get _hash);
                KH_var_allModulePostProcessing deleteAt _hash;
            };

            if _activated then {
                if ((_logic getVariable ["KH_ModuleLocalPostProcessingEffect", ""]) isNotEqualTo "") then {
                    private _area = _logic getVariable "objectarea";
                    private _color = _logic getVariable ["KH_ModuleLocalPostProcessingColor", [1, 1, 1]];

                    KH_var_allModulePostProcessing set [
                        _hash,
                        addLocalPostFX [
                            getPosWorldVisual _logic,
                            [_area select 0, _area select 1, _area select 4],
                            parseNumber (_logic getVariable ["KH_ModuleLocalPostProcessingFalloff", 0]),
                            _logic getVariable ["KH_ModuleLocalPostProcessingEffect", ""], 
                            parseSimpleArray (["[", _logic getVariable ["KH_ModuleLocalPostProcessingParameters", ""], "]"] joinString ""), 
                            [_color select 0, _color select 1, _color select 2, 1],
                            ["sphere", "cube"] select (_area select 3), 
                            _logic getVariable ["KH_ModuleLocalPostProcessingBlend", "NORMAL"],  
                            0,
                            _logic getVariable ["KH_ModuleLocalPostProcessingInverse", false]
                        ]
                    ];
                };
            };
        };

        case "unregisteredFromWorld3DEN": {
            _input params [["_logic", objNull, [objNull]]];
            private _hash = hashValue _logic;
            
            if !(KH_var_allModulePostProcessing isNil _hash) then {
                removeRenderHandler (KH_var_allModulePostProcessing get _hash);
                KH_var_allModulePostProcessing deleteAt _hash;
            };
        };

        default {
            _input params [["_logic", objNull, [objNull]]];
            private _hash = hashValue _logic;

            if !(KH_var_allModulePostProcessing isNil _hash) then {
                removeRenderHandler (KH_var_allModulePostProcessing get _hash);
            };

            if (((_logic get3DENAttribute "KH_ModuleLocalPostProcessingShow") select 0) && (((_logic get3DENAttribute "KH_ModuleLocalPostProcessingEffect") select 0) isNotEqualTo "")) then {
                private _area = (_logic get3DENAttribute "size3") select 0;
                private _color = (_logic get3DENAttribute "KH_ModuleLocalPostProcessingColor") select 0;

                KH_var_allModulePostProcessing set [
                    _hash,
                    addLocalPostFX [
                        getPosWorldVisual _logic,
                        _area select [0, 3],
                        parseNumber ((_logic get3DENAttribute "KH_ModuleLocalPostProcessingFalloff") select 0),
                        (_logic get3DENAttribute "KH_ModuleLocalPostProcessingEffect") select 0, 
                        parseSimpleArray (["[", (_logic get3DENAttribute "KH_ModuleLocalPostProcessingParameters") select 0, "]"] joinString ""), 
                        [_color select 0, _color select 1, _color select 2, 1],
                        ["sphere", "cube"] select ((_logic get3DENAttribute "isRectangle") select 0), 
                        (_logic get3DENAttribute "KH_ModuleLocalPostProcessingBlend") select 0,
                        0,
                        (_logic get3DENAttribute "KH_ModuleLocalPostProcessingInverse") select 0
                    ]
                ];
            };
        };
    };
};

nil;