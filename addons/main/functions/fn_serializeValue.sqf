params [["_type", "", ["", missionNamespace]], "_value"];

if (_type isEqualTo "") then {
    _type = typeName _value;
};

private _return = switch _type do {
    case "STRING";
    case "MARKER";
    case "TEXT": {
        _value;
    };

    case "BOOL";
    case "SCALAR": {
        str _value;
    };

    case "ARRAY": {
        private _invalidTypeCheckId = false;

        [
            _value,
            [],
            {
                if !(isNil "_x") then {
                    if !(_x isEqualTypeAny [true, 0, ""]) then {
                        _invalidTypeCheckId = true;
                        _breaker = true;
                    };
                };
            },
            false,
            [],
            ""
        ] call KH_fnc_nestedArrayOperation;

        if _invalidTypeCheckId then {
            ["TYPED_ARRAY", _value] call KH_fnc_serializeValue;
        }
        else {
            str _value;
        };
    };

    case "HASHMAP": {
        private _invalidTypeCheckId = false;

        [
            _value,
            [],
            {
                if !(isNil "_x") then {
                    if !(_x isEqualTypeAny [true, 0, ""]) then {
                        _invalidTypeCheckId = true;
                        _breaker = true;
                    };
                };
            },
            false,
            [],
            ""
        ] call KH_fnc_nestedArrayOperation;

        if _invalidTypeCheckId then {
            ["TYPED_HASHMAP", _value] call KH_fnc_serializeValue;
        }
        else {
            str (toArray _value);
        };
    };

    case "CODE": {
        toString _value;
    };

    case "OBJECT";
    case "GROUP";
    case "TEAM_MEMBER": {
        [_value, true] call KH_fnc_getEntityVariableName;
    };

    case "NAMESPACE": {
        switch _value do {
            case missionNamespace: {
                "missionNamespace";
            };

            case missionProfileNamespace: {
                "missionProfileNamespace";
            };

            case profileNamespace: {
                "profileNamespace";
            };

            case serverNamespace: {
                "serverNamespace";
            };

            case uiNamespace: {
                "uiNamespace";
            };

            case parsingNamespace: {
                "parsingNamespace";
            };
        };
    };

    case "SIDE": {
        switch _value do {
            case west: {
                "west";
            };

            case east: {
                "east";
            };

            case resistance: {
                "resistance";
            };

            case civilian: {
                "civilian";
            };

            case blufor: {
                "blufor";
            };

            case opfor: {
                "opfor";
            };

            case independent: {
                "independent";
            };

            case sideFriendly: {
                "sideFriendly";
            };

            case sideEnemy: {
                "sideEnemy";
            };

            case sideAmbientLife: {
                "sideAmbientLife";
            };

            case sideEmpty: {
                "sideEmpty";
            };

            case sideUnknown: {
                "sideUnknown";
            };

            case sideLogic: {
                "sideLogic";
            };
        };
    };

    case "LOCATION": {
        name _value;
    };

    case missionNamespace;
    case "MISSION_NAMESPACE_REFERENCE": {
        ["", missionNamespace getVariable _value] call KH_fnc_serializeValue;
    };

    case missionProfileNamespace;
    case "MISSION_PROFILE_NAMESPACE_REFERENCE": {
        ["", missionProfileNamespace getVariable _value] call KH_fnc_serializeValue;
    };
    
    case profileNamespace;
    case "PROFILE_NAMESPACE_REFERENCE": {
        ["", profileNamespace getVariable _value] call KH_fnc_serializeValue;
    };

    case uiNamespace;
    case "UI_NAMESPACE_REFERENCE": {
        ["", uiNamespace getVariable _value] call KH_fnc_serializeValue;
    };

    case serverNamespace;
    case "SERVER_NAMESPACE_REFERENCE": {
        ["", serverNamespace getVariable _value] call KH_fnc_serializeValue;
    };

    case parsingNamespace;
    case "PARSING_NAMESPACE_REFERENCE": {
        ["", parsingNamespace getVariable _value] call KH_fnc_serializeValue;
    };
    
    case "KH_DATA_NAMESPACE_REFERENCE": {
        ["kh_data_namespace", _value] call KH_fnc_readKhData;
    };

    case "LUA_NAMESPACE_REFERENCE": {
        [_value] call KH_fnc_luaGetVariable;
    };

    case "TYPED_ARRAY";
    case "TYPED_HASHMAP": {
        [
            _value,
            [],
            {
                _args params ["_array"];

                if ((_forEachIndex isEqualTo 0) || ((_forEachIndex % 2) isEqualTo 0)) then {
                    [_array select (_forEachIndex + 1), _array select (_forEachIndex + 1)] call KH_fnc_serializeValue;
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

if (isNil "_return") then {
    ["NIL", nil];
}
else {
    [_type, _return];
};