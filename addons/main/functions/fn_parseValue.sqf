params [["_parsingType", "", [""]], "_value"];

switch _parsingType do {
    case "BOOL";
    case "SCALAR";
    case "STRING": {
        _value;
    };

    case "TEXT": {
        text _value;
    };

    case "ARRAY": {
        _value;
    };

    case "HASHMAP": {
        createHashMapFromArray _value;
    };

    case "CODE": {
        missionNamespace getVariable ([_value, false] call KH_fnc_parseFunction);
    };

    case "OBJECT";
    case "TEAM_MEMBER";
    case "GROUP";
    case "LOCATION";
    case "MARKER": {
        [_parsingType, _value] call KH_fnc_getEntityByIdentifier;
    };

    case "NAMESPACE": {
        switch _value do {
            case "missionNamespace": {
                missionNamespace;
            };

            case "missionProfileNamespace": {
                missionProfileNamespace;
            };

            case "profileNamespace": {
                profileNamespace;
            };

            case "serverNamespace": {
                serverNamespace;
            };

            case "uiNamespace": {
                uiNamespace;
            };

            case "parsingNamespace": {
                parsingNamespace;
            };

            default {
                nil;
            };
        };
    };

    case "SIDE": {
        switch _value do {
            case "west": {
                west;
            };

            case "east": {
                east;
            };

            case "resistance": {
                resistance;
            };

            case "civilian": {
                civilian;
            };

            case "blufor": {
                blufor;
            };

            case "opfor": {
                opfor;
            };

            case "independent": {
                independent;
            };

            case "sideFriendly": {
                sideFriendly;
            };

            case "sideEnemy": {
                sideEnemy;
            };

            case "sideAmbientLife": {
                sideAmbientLife;
            };

            case "sideEmpty": {
                sideEmpty;
            };

            case "sideUnknown": {
                sideUnknown;
            };

            case "sideLogic": {
                sideLogic;
            };

            default {
                nil;
            };
        };
    };

    case "MISSION_NAMESPACE_REFERENCE": {
        missionNamespace getVariable _value;
    };

    case "MISSION_PROFILE_NAMESPACE_REFERENCE": {
        missionProfileNamespace getVariable _value;
    };

    case "PROFILE_NAMESPACE_REFERENCE": {
        profileNamespace getVariable _value;
    };

    case "UI_NAMESPACE_REFERENCE": {
        uiNamespace getVariable _value;
    };

    case "SERVER_NAMESPACE_REFERENCE": {
        serverNamespace getVariable _value;
    };
    
    case "PARSING_NAMESPACE_REFERENCE": {
        parsingNamespace getVariable _value;
    };
    
    case "KH_DATA_NAMESPACE_REFERENCE": {
        (["kh_data_namespace", _value] call KH_fnc_readKhData) call KH_fnc_parseValue;
    };

    case "LUA_NAMESPACE_REFERENCE": {
        [_value] call KH_fnc_luaGetVariable;
    };

    case "MISSION_CONFIG";
    case "CONFIG": {
        call (compile _value);
    };

    case "TYPED_ARRAY": {
        [
            _value,
            [],
            {
                _args params ["_array"];

                if ((_forEachIndex % 2) isEqualTo 0) then {
                    [_x, _array select (_forEachIndex + 1)] call KH_fnc_parseValue;
                };
            },
            true,
            [],
            ""
        ] call KH_fnc_nestedArrayOperation;
    };

    case "TYPED_HASHMAP": {
        [
            createHashMapFromArray _value,
            [],
            {
                _args params ["_array"];

                if ((_forEachIndex % 2) isEqualTo 0) then {
                    [_x, _array select (_forEachIndex + 1)] call KH_fnc_parseValue;
                };
            },
            true,
            [],
            ""
        ] call KH_fnc_nestedArrayOperation;
    };

    default {
        nil;
    };
};