params [["_type", "", [""]], ["_value", nil, [true, 0, "", text "", [], createHashMap, {}, objNull, teamMemberNull, grpNull, sideUnknown, missionNamespace, locationNull, configNull]]];

if (isNil "_value") exitWith {
    ["NIL", nil];
};

if (_type isEqualTo "") then {
    _type = typeName _value;
};

_value = switch _type do {
    case "BOOL";
    case "SCALAR";
    case "STRING";
    case "TEXT";
    case "MARKER": {
        _value;
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
            _value;
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
            toArray _value;
        };
    };

    case "CODE": {
        toString _value;
    };

    case "OBJECT";
    case "TEAM_MEMBER";
    case "GROUP": {
        if !(isNull _value) then {
            [_value, true] call KH_fnc_getEntityVariableName;
        };
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

            default {
                nil;
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

            default {
                nil;
            };
        };
    };

    case "LOCATION": {
        if !(isNull _value) then {
            name _value;
        };
    };

    case "MISSION_NAMESPACE_REFERENCE": {
        ["", missionNamespace getVariable _value] call KH_fnc_serializeValue;
    };

    case "MISSION_PROFILE_NAMESPACE_REFERENCE": {
        ["", missionProfileNamespace getVariable _value] call KH_fnc_serializeValue;
    };
    
    case "PROFILE_NAMESPACE_REFERENCE": {
        ["", profileNamespace getVariable _value] call KH_fnc_serializeValue;
    };

    case "UI_NAMESPACE_REFERENCE": {
        ["", uiNamespace getVariable _value] call KH_fnc_serializeValue;
    };

    case "SERVER_NAMESPACE_REFERENCE": {
        ["", serverNamespace getVariable _value] call KH_fnc_serializeValue;
    };

    case "PARSING_NAMESPACE_REFERENCE": {
        ["", parsingNamespace getVariable _value] call KH_fnc_serializeValue;
    };
    
    case "KH_DATA_NAMESPACE_REFERENCE": {
        ["kh_data_namespace", _value] call KH_fnc_readKhData;
    };

    case "LUA_NAMESPACE_REFERENCE": {
        [_value] call KH_fnc_luaGetVariable;
    };
    
    case "MISSION_CONFIG": {
        if !(isNull _value) then {
            private _hierarchy = configHierarchy _value;
            private _path = [];

            {   
                _path pushBack (_x regexFind [".*/([^/]*)"]);
            } forEach ((configHierarchy _value) select [1]);

            ["missionConfigFile", " >> '", _path joinString "' >> '", "'"] joinString "";
        };
    };

    case "CONFIG": {
        if !(isNull _value) then {
            private _hierarchy = configHierarchy _value;
            private _path = [];

            {   
                _path pushBack (_x regexFind [".*/([^/]*)"]);
            } forEach ((configHierarchy _value) select [1]);

            ["configFile", " >> '", _path joinString "' >> '", "'"] joinString "";
        };
    };

    case "TYPED_ARRAY";
    case "TYPED_HASHMAP": {
        [
            _value,
            [],
            {
                _args params ["_array"];

                if ((_forEachIndex % 2) isEqualTo 0) then {
                    [_x, _array select (_forEachIndex + 1)] call KH_fnc_serializeValue;
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

if (isNil "_value") then {
    ["NIL", nil];
}
else {
    [_type, _value];
};