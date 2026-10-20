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
                if ((_logic getVariable ["KH_ModulePostProcessingEffect", ""]) isNotEqualTo "") then {
                    private _color = _logic getVariable ["KH_ModulePostProcessingColor", [1, 1, 1]];
                    
                    KH_var_allModulePostProcessing set [
                        _hash,
                        addPostFX [
                            _logic getVariable ["KH_ModulePostProcessingEffect", ""], 
                            parseSimpleArray (["[", _logic getVariable ["KH_ModulePostProcessingParameters", ""], "]"] joinString ""), 
                            [_color select 0, _color select 1, _color select 2, 1], 
                            [
                                parseNumber (_logic getVariable ["KH_ModulePostProcessingMinimumDistance", "0"]),
                                parseNumber (_logic getVariable ["KH_ModulePostProcessingMaximumDistance", "0"]),
                                parseNumber (_logic getVariable ["KH_ModulePostProcessingFalloff", "10"])
                            ], 
                            _logic getVariable ["KH_ModulePostProcessingBlend", "NORMAL"], 
                            _logic getVariable ["KH_ModulePostProcessingType", "SCENE"], 
                            0
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

            if (((_logic get3DENAttribute "KH_ModulePostProcessingShow") select 0) && (((_logic get3DENAttribute "KH_ModulePostProcessingEffect") select 0) isNotEqualTo "")) then {
                private _color = (_logic get3DENAttribute "KH_ModulePostProcessingColor") select 0;

                KH_var_allModulePostProcessing set [
                    _hash,
                    addPostFX [
                        (_logic get3DENAttribute "KH_ModulePostProcessingEffect") select 0, 
                        parseSimpleArray (["[", (_logic get3DENAttribute "KH_ModulePostProcessingParameters") select 0, "]"] joinString ""), 
                        [_color select 0, _color select 1, _color select 2, 1], 
                        [
                            parseNumber ((_logic get3DENAttribute "KH_ModulePostProcessingMinimumDistance") select 0),
                            parseNumber ((_logic get3DENAttribute "KH_ModulePostProcessingMaximumDistance") select 0),
                            parseNumber ((_logic get3DENAttribute "KH_ModulePostProcessingFalloff") select 0)
                        ], 
                        (_logic get3DENAttribute "KH_ModulePostProcessingBlend") select 0, 
                        (_logic get3DENAttribute "KH_ModulePostProcessingType") select 0, 
                        0
                    ]
                ];
            };
        };
    };
};

nil;