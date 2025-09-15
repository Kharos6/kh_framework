params [["_type", "", [""]], ["_config", "", ["", [], {}, objNull, configNull]]];

if !(_config isEqualType configNull) then {
    _config = switch (typeName _config) do {
        case "STRING": {
            call (compile _config);
        };

        case "ARRAY": {
            _config params [["_path", "", [""]], ["_prioritizeMissionConfig", true, [true]]];
            
            if _prioritizeMissionConfig then {
                private _missionConfig = call (compile (["missionConfigFile", _path] joinString " >> "));

                if !(isNull _missionConfig) then {
                    _missionConfig;
                }
                else {
                    call (compile (["configFile", _path] joinString " >> "));
                };
            }
            else {
                call (compile (["configFile", _path] joinString " >> "));
            };
        };

        case "CODE": {
            call _config;
        };

        case "OBJECT": {
            configOf _config;
        };

        default {
            configNull;
        };
    };
};

if (isNull _config) exitWith {};

if (_type isEqualTo "") then {
    switch true do {
        case (isNumber _config): {
            getNumber _config;
        };

        case (isText _config): {
            getText config;
        };

        case (isArray _config): {
            getArray _config;
        };

        case (isClass _config): {
            "true" configClasses _config;
        };

        default {
            nil;
        };
    };
}
else {
    switch _type do {
        case "BOOL": {
            [getNumber _config, false] call KH_fnc_parseBoolean;
        };

        case "SCALAR": {
            getNumber _config;
        };

        case "STRING": {
            getText _config;
        };

        case "TEXT": {
            getTextRaw _config;
        };

        case "ARRAY": {
            getArray _config;
        };

        case "HASHMAP": {
            private _array = getArray _config;
            private _hashMap = createHashMap;

            {
                if ((_forEachIndex % 2) isEqualTo 0) then {
                    _hashMap set [_x, _array select (_forEachIndex + 1)];
                };
            } forEach _array;

            _hashMap;
        };

        case "CODE": {
            missionNamespace getVariable ([getText _config, false] call KH_fnc_parseFunction);
        };

        case "CLASSES": {
            "true" configClasses _config;
        };
        
        default {
            nil;
        };
    };
};