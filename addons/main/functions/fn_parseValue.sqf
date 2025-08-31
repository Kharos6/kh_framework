params [["_parsingType", "", ["", missionNamespace]], ["_value", "", [""]]];

switch _parsingType do {
    case "BOOL": {
        if (_value isEqualType "") then {
            [_value, false] call KH_fnc_parseBoolean;
        }
        else {
            _value;
        };
    };

    case "SCALAR": {
        if (_value isEqualType 0) then {
            _value;
        }
        else {
            parseNumber _value;
        };
    };

    case "STRING": {
        if (_value isEqualType "") then {
            _value;
        }
        else {
            str _value;
        };
    };

    case "TEXT": {
        if (_value isEqualType (text _value)) then {
            _value;
        }
        else {
            text (str _value);
        };
    };

    case "ARRAY": {
        if (_value isEqualType "") then {
            parseSimpleArray _value;
        }
        else {
            _value;
        };
    };

    case "HASHMAP": {
        if (_value isEqualType "") then {
            createHashMapFromArray (parseSimpleArray _value);
        }
        else {
            createHashMapFromArray _value;
        };
    };

    case "CODE": {
        missionNamespace getVariable ([_value, false] call KH_fnc_parseFunction);
    };

    case "OBJECT";
    case "GROUP";
    case "TEAM_MEMBER";
    case "MARKER";
    case "LOCATION": {
        [_value, _parsingType] call KH_fnc_getEntityByIdentifier;
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
        };
    };

    case "LOCATION": {
       ((nearestLocations [[worldSize / 2, worldSize / 2], [], worldSize * sqrt 2 / 2]) select {((_value isEqualTo (name _x)) || (_value isEqualTo (className _x)))}) param [0];
    };

    case missionNamespace;
    case "MISSION_NAMESPACE_REFERENCE": {
        missionNamespace getVariable _value;
    };

    case missionProfileNamespace;
    case "MISSION_PROFILE_NAMESPACE_REFERENCE": {
        missionProfileNamespace getVariable _value;
    };

    case profileNamespace;
    case "PROFILE_NAMESPACE_REFERENCE": {
        profileNamespace getVariable _value;
    };

    case uiNamespace;
    case "UI_NAMESPACE_REFERENCE": {
        uiNamespace getVariable _value;
    };

    case serverNamespace;
    case "SERVER_NAMESPACE_REFERENCE": {
        serverNamespace getVariable _value;
    };
    
    case parsingNamespace;
    case "PARSING_NAMESPACE_REFERENCE": {
        parsingNamespace getVariable _value;
    };
    
    case "KH_DATA_NAMESPACE_REFERENCE": {
        (["kh_data_namespace", _value] call KH_fnc_readKhData) call KH_fnc_parseValue;
    };

    case "LUA_NAMESPACE_REFERENCE": {
        [_value] call KH_fnc_luaGetVariable;
    };

    case "TYPED_ARRAY": {
        [
            if (_value isEqualType "") then {
                parseSimpleArray _value;
            }
            else {
                _value;
            },
            [],
            {
                _args params ["_array"];

                if ((_forEachIndex isEqualTo 0) || ((_forEachIndex % 2) isEqualTo 0)) then {
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
            if (_value isEqualType "") then {
                createHashMapFromArray (parseSimpleArray _value);
            }
            else {
                _value;
            },
            [],
            {
                _args params ["_array"];

                if ((_forEachIndex isEqualTo 0) || ((_forEachIndex % 2) isEqualTo 0)) then {
                    [_x, _array select (_forEachIndex + 1)] call KH_fnc_parseValue;
                };
            },
            true,
            [],
            ""
        ] call KH_fnc_nestedArrayOperation;
    };
};