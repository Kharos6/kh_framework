params ["_arguments", "_function", "_target", "_sendoffArguments", "_sendoffFunction", "_executionOwner", "_id"];

[
    ["STANDARD", _target, "LOCAL"],
    "Local",
    [_arguments, _function, _sendoffArguments, _sendoffFunction, _executionOwner, _id],
    {
        params ["_target", "_isLocal"];
        _args params ["_arguments", "_function", "_sendoffArguments", "_sendoffFunction", "_executionOwner", "_id"];
        private _idState = _target getVariable [_id, "ACTIVE"];

        if (_idState isNotEqualTo "INACTIVE") then {
            switch _idState do {
                case "ACTIVE": {
                    if _isLocal then {
                        [_arguments, _function, _executionOwner] call KH_fnc_callParsedFunction;
                    }
                    else {
                        [_sendoffArguments, _sendoffFunction, _executionOwner] call KH_fnc_callParsedFunction;
                    };
                };

                case "TERMINATE": {
                    [_localId] call KH_fnc_removeEventHandler;
                };
            };
        };
    }
] call KH_fnc_addEventHandler;