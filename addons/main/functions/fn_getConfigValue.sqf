params [["_type", "", [""]], ["_config", configNull, ["", [], {}, objNull, teamMemberNull, configNull]], "_default"];

if !(_config isEqualType configNull) then {
    _config = switch (typeName _config) do {
        case "STRING": {
            call (compile _config);
        };

        case "ARRAY": {
            _config params [["_path", "", [""]], ["_configPriority", ["BASE"], [[]]]];
            private _currentConfig = configNull;

            {
                switch _x do {
                    case "MISSION": {
                        _currentConfig = call (compile (["missionConfigFile", _path] joinString " >> "));

                        if !(isNull _currentConfig) then {
                            break;
                        };
                    };

                    case "CAMPAIGN": {
                        _currentConfig = call (compile (["campaignConfigFile", _path] joinString " >> "));

                        if !(isNull _currentConfig) then {
                            break;
                        };
                    };

                    case "BASE": {
                        _currentConfig = call (compile (["configFile", _path] joinString " >> "));
                        
                        if !(isNull _currentConfig) then {
                            break;
                        };
                    };
                };
            } forEach _configPriority;

            _currentConfig;
        };

        case "CODE": {
            call _config;
        };

        case "OBJECT": {
            configOf _config;
        };

        case "TEAM_MEMBER": {
            configOf (agent _config);
        };

        default {
            configNull;
        };
    };
};

if !(isNull _config) then {
    if (_type isEqualTo "") then {
        switch true do {
            case (isNumber _config): {
                getNumber _config;
            };

            case (isText _config): {
                getText _config;
            };

            case (isArray _config): {
                getArray _config;
            };

            case (isClass _config): {
                _config;
            };

            default {
                _default;
            };
        };
    }
    else {
        switch _type do {
            case "BOOL": {
                if (isNumber _config) then {
                    [getNumber _config, false] call KH_fnc_parseBoolean;
                }
                else {
                    _default;
                };
            };

            case "SCALAR": {
                if (isNumber _config) then {
                    getNumber _config;
                }
                else {
                    _default;
                };
            };

            case "STRING": {
                if (isText _config) then {
                    getText _config;
                }
                else {
                    _default;
                };
            };

            case "TEXT": {
                if (isText _config) then {
                    getTextRaw _config;
                }
                else {
                    _default;
                };
            };

            case "ARRAY": {
                if (isArray _config) then {
                    getArray _config;
                }
                else {
                    _default;
                };
            };

            case "HASHMAP": {
                if (isArray _config) then {
                    private _array = getArray _config;
                    private _hashMap = createHashMap;

                    {
                        if ((_forEachIndex % 2) isEqualTo 0) then {
                            _hashMap set [_x, _array select (_forEachIndex + 1)];
                        };
                    } forEach _array;

                    _hashMap;
                }
                else {
                    _default;
                };
            };

            case "CODE": {
                if (isText _config) then {
                    missionNamespace getVariable (false serializeFunction (getText _config));
                }
                else {
                    _default;
                };
            };

            case "CLASS": {
                [_default, _config] select (isClass _config);
            };

            case "CLASSES": {
                if (isClass _config) then {
                    "true" configClasses _config;
                }
                else {
                    _default;
                };
            };

            case "PROPERTIES": {
                if (isClass _config) then {
                    configProperties [_config, "true", true];
                }
                else {
                    _default;
                };
            };

            case "HIERARCHY": {
                if (isClass _config) then {
                    configHierarchy [_config, true, true, false, false];
                }
                else {
                    _default;
                };
            };
            
            default {
                _default;
            };
        };
    };
}
else {
    _default;
};