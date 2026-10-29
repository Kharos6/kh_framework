private _entity = param [0];
if !(_entity isEqualType objNull) exitWith {};

{
    {
        deleteVehicle (_x select 0);
    } forEach _y;
} forEach (_entity getVariable ["KH_var_attachedObjectHandlers", createHashMap]);

private _objectHandlers = createHashMap;
_entity setVariable ["KH_var_attachedObjectHandlers", _objectHandlers];

if (_entity isKindOf "Man") then {
    [
        ["ENTITY", _entity, "LOCAL"],
        "SlotItemChanged",
        [],
        {
            params ["_entity", "_name", "_slot", "_assigned"];
            private _objectHandlers = _entity getVariable ["KH_var_attachedObjectHandlers", createHashMap];
            private _key = str _slot;

            if !(_objectHandlers isNil _key) then {
                {
                    deleteVehicle (_x select 0);
                } forEach (_objectHandlers get _key);

                _objectHandlers deleteAt _key;
            };
            
            if _assigned then {
                private _currentHandlers = [];

                {
                    private _objectHandler = createSimpleObject [getText (_x >> "model"), getPosASLVisual _entity, true];
                    _objectHandler attachTo [_entity, getArray (_x >> "offset"), getText (_x >> "memoryPoint"), (getNumber (_x >> "followBoneRotation")) isEqualTo 1];

                    if !(isNull (objectParent _entity)) then {
                        _entity setVariable [
                            "KH_var_attachedObjectInVehicleHandler",
                            execute [
                                [_entity],
                                {
                                    params ["_entity"];
                                    
                                    if (isNull _entity) exitWith {
                                        [_handlerId] call KH_fnc_removeHandler;
                                    };

                                    {
                                        {
                                            _x params ["_objectHandler", "_class"];
                                            _objectHandler setPosASL (_entity modelToWorldVisualWorld (_entity selectionPosition [getText (_class >> "memoryPoint"), "Memory"]));
                                            _objectHandler setVectorDirAndUp (_entity vectorModelToWorldVisual (_entity selectionVectorDirAndUp [getText (_class >> "memoryPoint"), "Memory"]));
                                        } forEach _x;
                                    } forEach (values (_entity getVariable ["KH_var_attachedObjectHandlers", createHashMap]));
                                },
                                true,
                                0,
                                false
                            ]                
                        ];

                        if ((getNumber (_x >> "hideInVehicle")) isEqualTo 1) then {
                            _objectHandler hideObject true;
                        };
                    };
                    
                    _currentHandlers pushBack [_objectHandler, _x];
                } forEach (
                    "true" configClasses (
                        switch _slot do {
                            case 603: {
                                configFile >> "CfgGlasses" >> _name >> "KH_AttachedObjects";
                            };

                            case 901: {
                                configFile >> "CfgVehicles" >> _name >> "KH_AttachedObjects";
                            };

                            default {
                                configFile >> "CfgWeapons" >> _name >> "KH_AttachedObjects";
                            };
                        }
                    )
                );

                if (_currentHandlers isNotEqualTo []) then {
                    _objectHandlers set [_key, _currentHandlers];
                };

                if ((keys _objectHandlers) isEqualTo []) then {
                    [_entity getVariable ["KH_var_attachedObjectInVehicleHandler", []]] call KH_fnc_removeHandler;
                };
            };                    
        },
        ["KH_var_attachedObjectSlotItemHandler", hashValue _entity] joinString "_"
    ] call KH_fnc_addEventHandler;

    [
        ["ENTITY", _entity, "LOCAL"],
        "GetInMan",
        [],
        {
            params ["_entity"];

            {
                {
                    _x params ["_objectHandler", "_class"];

                    if ((getNumber (_class >> "hideInVehicle")) isEqualTo 1) then {
                        _objectHandler hideObject true;
                    };

                    detach _objectHandler;
                } forEach _x;
            } forEach (values (_entity getVariable ["KH_var_attachedObjectHandlers", createHashMap]));

            _entity setVariable [
                "KH_var_attachedObjectInVehicleHandler",
                execute [
                    [_entity],
                    {
                        params ["_entity"];
                        
                        if (isNull _entity) exitWith {
                            [_handlerId] call KH_fnc_removeHandler;
                        };

                        {
                            {
                                _x params ["_objectHandler", "_class"];
                                _objectHandler setPosASL (_entity modelToWorldVisualWorld (_entity selectionPosition [getText (_class >> "memoryPoint"), "Memory"]));
                                _objectHandler setVectorDirAndUp (_entity vectorModelToWorldVisual (_entity selectionVectorDirAndUp [getText (_class >> "memoryPoint"), "Memory"]));
                            } forEach _x;
                        } forEach (values (_entity getVariable ["KH_var_attachedObjectHandlers", createHashMap]));
                    },
                    true,
                    0,
                    false
                ]                
            ];
        },
        ["KH_var_attachedObjectGetInHandler", hashValue _entity] joinString "_"
    ] call KH_fnc_addEventHandler;

    [
        ["ENTITY", _entity, "LOCAL"],
        "GetOutMan",
        [],
        {
            params ["_entity"];
            [_entity getVariable ["KH_var_attachedObjectInVehicleHandler", []]] call KH_fnc_removeHandler;

            {
                {
                    _x params ["_objectHandler", "_class"];
                    _objectHandler attachTo [_entity, getArray (_class >> "offset"), getText (_class >> "memoryPoint"), (getNumber (_class >> "followBoneRotation")) isEqualTo 1];
                } forEach _x;
            } forEach (values (_entity getVariable ["KH_var_attachedObjectHandlers", createHashMap]));        
        },
        ["KH_var_attachedObjectGetOutHandler", hashValue _entity] joinString "_"
    ] call KH_fnc_addEventHandler;

    [
        "CBA",
        "KH_eve_weaponSlotChanged",
        [_entity],
        {
            params ["_entity", "_slot", "_newWeapon"];
            _args params ["_thisEntity"];
            if (_entity isNotEqualTo _thisEntity) exitWith {};
            private _objectHandlers = _entity getVariable ["KH_var_attachedObjectHandlers", createHashMap];

            if !(_objectHandlers isNil _slot) then {
                {
                    deleteVehicle (_x select 0);
                } forEach (_objectHandlers get _slot);

                _objectHandlers deleteAt _slot;
            };

            private _currentHandlers = [];

            {
                private _objectHandler = createSimpleObject [getText (_x >> "model"), getPosASLVisual _entity, true];
                _objectHandler attachTo [_entity, getArray (_x >> "offset"), getText (_x >> "memoryPoint"), (getNumber (_x >> "followBoneRotation")) isEqualTo 1];

                if !(isNull (objectParent _entity)) then {
                    _entity setVariable [
                        "KH_var_attachedObjectInVehicleHandler",
                        execute [
                            [_entity],
                            {
                                params ["_entity"];
                                
                                if (isNull _entity) exitWith {
                                    [_handlerId] call KH_fnc_removeHandler;
                                };

                                {
                                    {
                                        _x params ["_objectHandler", "_class"];
                                        _objectHandler setPosWorld (_entity modelToWorldVisualWorld (_entity selectionPosition [getText (_class >> "memoryPoint"), "Memory"]));
                                        _objectHandler setVectorDirAndUp (_entity vectorModelToWorldVisual (_entity selectionVectorDirAndUp [getText (_class >> "memoryPoint"), "Memory"]));
                                    } forEach _x;
                                } forEach (values (_entity getVariable ["KH_var_attachedObjectHandlers", createHashMap]));
                            },
                            true,
                            0,
                            false
                        ]                
                    ];

                    if ((getNumber (_x >> "hideInVehicle")) isEqualTo 1) then {
                        _objectHandler hideObject true;
                    };
                };

                _currentHandlers pushBack [_objectHandler, _x];
            } forEach ("true" configClasses (configFile >> "CfgWeapons" >> _newWeapon >> "KH_AttachedObjects"));

            if (_currentHandlers isNotEqualTo []) then {
                _objectHandlers set [_slot, _currentHandlers];
            };

            if ((keys _objectHandlers) isEqualTo []) then {
                [_entity getVariable ["KH_var_attachedObjectInVehicleHandler", []]] call KH_fnc_removeHandler;
            };
        },
        ["KH_var_attachedObjectWeaponSlotHandler", hashValue _entity] joinString "_"
    ] call KH_fnc_addEventHandler;
};

private _currentHandlers = [];

{
    private _objectHandler = createSimpleObject [getText (_x >> "model"), getPosASLVisual _entity, true];
    _objectHandler attachTo [_entity, getArray (_x >> "offset"), getText (_x >> "memoryPoint"), (getNumber (_x >> "followBoneRotation")) isEqualTo 1];

    if !(isNull (objectParent _entity)) then {
        _entity setVariable [
            "KH_var_attachedObjectInVehicleHandler",
            execute [
                [_entity],
                {
                    params ["_entity"];
                    
                    if (isNull _entity) exitWith {
                        [_handlerId] call KH_fnc_removeHandler;
                    };

                    {
                        {
                            _x params ["_objectHandler", "_class"];
                            _objectHandler setPosWorld (_entity modelToWorldVisualWorld (_entity selectionPosition [getText (_class >> "memoryPoint"), "Memory"]));
                            _objectHandler setVectorDirAndUp (_entity vectorModelToWorldVisual (_entity selectionVectorDirAndUp [getText (_class >> "memoryPoint"), "Memory"]));
                        } forEach _x;
                    } forEach (values (_entity getVariable ["KH_var_attachedObjectHandlers", createHashMap]));
                },
                true,
                0,
                false
            ]                
        ];

        if ((getNumber (_x >> "hideInVehicle")) isEqualTo 1) then {
            _objectHandler hideObject true;
        };
    };

    _currentHandlers pushBack [_objectHandler, _x];
} forEach ("true" configClasses ((configOf _entity) >> "KH_AttachedObjects"));

if (_currentHandlers isNotEqualTo []) then {
    _objectHandlers set ["0", _currentHandlers];
};

if ((keys _objectHandlers) isEqualTo []) then {
    [_entity getVariable ["KH_var_attachedObjectInVehicleHandler", []]] call KH_fnc_removeHandler;
};

_objectHandlers;