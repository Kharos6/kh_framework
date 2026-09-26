private _entity = param [0];
if !(_entity isEqualType objNull) exitWith {};

{
    {
        removeRenderHandler (_x select 0);
    } forEach _y;
} forEach (_entity getVariable ["KH_var_renderHandlers", createHashMap]);

private _renderHandlers = createHashMap;
_entity setVariable ["KH_var_renderHandlers", _renderHandlers];

if (_entity isKindOf "Man") then {
    [
        ["ENTITY", _entity, "LOCAL"],
        "SlotItemChanged",
        [],
        {
            params ["_entity", "_name", "_slot", "_assigned"];
            private _renderHandlers = _entity getVariable ["KH_var_renderHandlers", createHashMap];
            private _key = str _slot;

            if !(_renderHandlers isNil _key) then {
                {
                    removeRenderHandler (_x select 0);
                } forEach (_renderHandlers get _key);

                _renderHandlers set [_key, []];
            };
            
            if _assigned then {
                private _currentHandlers = [];

                {
                    private _renderHandler = addRender3D [[_entity, ["", _x >> "bindSkeleton", true] call KH_fnc_getConfigValue], (getNumber (_x >> "followRotation")) isEqualTo 1, getText (_x >> "model")];

                    private _properties = ((configProperties [
                        _x, 
                        "!((toLowerANSI (configName _x)) in ['model', 'mesh', 'bindskeleton', 'followrotation']);",
                        true
                    ]) apply {
                        [_renderHandler, configName _x, ["", _x, nil] call KH_fnc_getConfigValue];
                    });

                    if (_properties isNotEqualTo []) then {
                        updateRender3D _properties;
                    };

                    if (((getNumber (_x >> "showInVehicle")) isEqualTo 0) && !(isNull (objectParent _entity))) then {
                        updateRender3D [_renderHandler, "visible", false];
                    };

                    _currentHandlers pushBack [_renderHandler, _x];
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
        },
        ["KH_var_renderingSlotItemHandler", hashValue _entity] joinString "_"
    ] call KH_fnc_addEventHandler;

    [
        ["ENTITY", _entity, "LOCAL"],
        "GetInMan",
        [],
        {
            params ["_entity"];

            {
                _x params ["_renderHandler", "_class"];

                if ((getNumber (_class >> "showInVehicle")) isEqualTo 0) then {
                    updateRender3D [_renderHandler, "visible", false];
                };
            } forEach (values (_entity getVariable ["KH_var_renderHandlers", createHashMap]));    
        },
        ["KH_var_renderingGetInHandler", hashValue _entity] joinString "_"
    ] call KH_fnc_addEventHandler;

    [
        ["ENTITY", _entity, "LOCAL"],
        "GetOutMan",
        [],
        {
            params ["_entity"];

            {
                _x params ["_renderHandler", "_class"];

                if ((getNumber (_class >> "showInVehicle")) isEqualTo 0) then {
                    updateRender3D [_renderHandler, "visible", true];
                };
            } forEach (values (_entity getVariable ["KH_var_renderHandlers", createHashMap]));        
        },
        ["KH_var_renderingGetOutHandler", hashValue _entity] joinString "_"
    ] call KH_fnc_addEventHandler;

    [
        "CBA",
        "KH_eve_weaponSlotChanged",
        [_entity],
        {
            params ["_entity", "_slot", "_newWeapon"];
            _args params ["_thisEntity"];
            if (_entity isNotEqualTo _thisEntity) exitWith {};
            private _renderHandlers = _entity getVariable ["KH_var_renderHandlers", createHashMap];

            if !(_renderHandlers isNil _slot) then {
                {
                    removeRenderHandler (_x select 0);
                } forEach (_renderHandlers get _slot);

                _renderHandlers set [_slot, []];
            };

            private _currentHandlers = [];

            {
                private _renderHandler = addRender3D [[_entity, ["", _x >> "bindSkeleton", true] call KH_fnc_getConfigValue], (getNumber (_x >> "followRotation")) isEqualTo 1, getText (_x >> "model")];

                private _properties = ((configProperties [
                    _x, 
                    "!((toLowerANSI (configName _x)) in ['model', 'mesh', 'bindskeleton', 'followrotation']);",
                    true
                ]) apply {
                    [_renderHandler, configName _x, ["", _x, nil] call KH_fnc_getConfigValue];
                });

                if (_properties isNotEqualTo []) then {
                    updateRender3D _properties;
                };

                _currentHandlers pushBack [_renderHandler, _x];
            } forEach ("true" configClasses (configFile >> "CfgWeapons" >> _newWeapon >> "KH_Rendering3D"));

            _renderHandlers set [_slot, _currentHandlers];                 
        },
        ["KH_var_renderingWeaponSlotHandler", hashValue _entity] joinString "_"
    ] call KH_fnc_addEventHandler;
};

private _currentHandlers = [];

{
    private _renderHandler = if (!is3DEN && ((getNumber (_x >> "static")) isEqualTo 1)) then {
        addRender3D [getPosWorldVisual _entity, [vectorDirVisual _entity, vectorUpVisual _entity], getText (_x >> "model")];
    }
    else {
        addRender3D [[_entity, ["", _x >> "bindSkeleton", true] call KH_fnc_getConfigValue], (getNumber (_x >> "followRotation")) isEqualTo 1, getText (_x >> "model")];
    };
    
    private _properties = ((configProperties [
        _x, 
        "!((toLowerANSI (configName _x)) in ['model', 'mesh', 'bindskeleton', 'followrotation']);",
        true
    ]) apply {
        [_renderHandler, configName _x, ["", _x, nil] call KH_fnc_getConfigValue];
    });

    if (_properties isNotEqualTo []) then {
        updateRender3D _properties;
    };

    if (((getNumber (_x >> "showInVehicle")) isEqualTo 0) && !(isNull (objectParent _entity))) then {
        updateRender3D [_renderHandler, "visible", false];
    };

    _currentHandlers pushBack [_renderHandler, _x];
} forEach ("true" configClasses ((configOf _entity) >> "KH_Rendering3D"));

if (_currentHandlers isNotEqualTo []) then {
    _renderHandlers set ["0", _currentHandlers];
};

_renderHandlers;