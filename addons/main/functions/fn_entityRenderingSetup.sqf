private _entity = param [0];
if !(_entity isEqualType objNull) exitWith {};
private _renderHandlers = createHashMap;
_entity setVariable ["KH_var_renderHandlers", _renderHandlers];

if (_entity isKindOf "Man") then {
    [
        ["ENTITY", _entity, "LOCAL"],
        "SlotItemChanged",
        [],
        {
            params ["_entity", "_name", "_slot", "_assigned", "_weapon"];
            private _renderHandlers = _entity getVariable ["KH_var_renderHandlers", createHashMap];
            private _key = str _slot;

            if !(_renderHandlers isNil _key) then {
                {
                    removeRenderHandler _x;
                } forEach (_renderHandlers get _key);

                _renderHandlers set [_key, []];
            };
            
            if _assigned then {
                private _currentHandlers = [];

                {
                    private _renderHandler = addRender3D [[_entity, (getNumber (_x >> "bindSkeleton")) isEqualTo 1], (getNumber (_x >> "followRotation")) isEqualTo 1, getText (_x >> "model")];

                    private _properties = ((configProperties [
                        _x, 
                        "!((toLowerANSI (configName _x)) in ['model', 'bindskeleton', 'followrotation']);",
                        true
                    ]) apply {
                        [_renderHandler, configName _x, ["", _x, nil] call KH_fnc_getConfigValue];
                    });

                    if (_properties isNotEqualTo []) then {
                        updateRender3D _properties;
                    };

                    _currentHandlers pushBack _renderHandler;
                } forEach (
                    "true" configClasses (
                        switch _slot do {
                            case 603: {
                                configFile >> "CfgGlasses" >> _name >> "KH_Rendering3D";
                            };

                            case 901: {
                                configFile >> "CfgVehicles" >> _name >> "KH_Rendering3D";
                            };

                            default {
                                configFile >> "CfgWeapons" >> _name >> "KH_Rendering3D";
                            };
                        }
                    )
                );

                _renderHandlers set [_key, _currentHandlers];
            };                    
        }
    ] call KH_fnc_addEventHandler;
};

private _currentHandlers = [];

{
    private _renderHandler = addRender3D [[_entity, (getNumber (_x >> "bindSkeleton")) isEqualTo 1], (getNumber (_x >> "followRotation")) isEqualTo 1, getText (_x >> "model")];
    
    private _properties = ((configProperties [
        _x, 
        "!((toLowerANSI (configName _x)) in ['model', 'bindskeleton', 'followrotation']);",
        true
    ]) apply {
        [_renderHandler, configName _x, ["", _x, nil] call KH_fnc_getConfigValue];
    });

    if (_properties isNotEqualTo []) then {
        updateRender3D _properties;
    };

    _currentHandlers pushBack _renderHandler;
} forEach ("true" configClasses ((configOf _entity) >> "KH_Rendering3D"));

if (_currentHandlers isNotEqualTo []) then {
    _renderHandlers set ["0", _currentHandlers];
};

_renderHandlers;