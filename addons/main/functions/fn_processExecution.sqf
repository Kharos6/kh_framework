params [
	"_arguments", 
	["_function", {}, ["", {}]], 
	["_target", true, [true, 0, "", [], {}, objNull, teamMemberNull, grpNull, sideUnknown, locationNull]],
	["_special", false, [true, [], createHashMap]],
    ["_specialIdOverride", "", [""]],
    ["_unscheduled", true, [true]]
];

if (_special isEqualTo false) exitWith {
    ["KH_eve_execution", [_arguments, _function, clientOwner, _unscheduled], _target, false] call KH_fnc_triggerCbaEvent;
};

if (_special isEqualType createHashMap) exitWith {
    private _argumentsId = generateUid;
    missionNamespace setVariable [_argumentsId, _arguments];

    [
        "KH_eve_execution", 
        [
            _arguments, 
            [compile ([_special, " call ['", _function, "', missionNamespace getVariable '", _argumentsId, "'];"] joinString ""), false] call KH_fnc_serializeFunction, 
            clientOwner, 
            true
        ], 
        _target, 
        false
    ] call KH_fnc_triggerCbaEvent;
};

private _specialType = _special param [0, "", [""]];

switch _specialType do {
    case "JIP": {
        private _dependency = _special param [1, true, [true, 0, "", [], {}, objNull, teamMemberNull, grpNull]];
        private _unitRequired = _special param [2, false, [true]];
        private _jipId = _special param [3, "", [""]];

        _jipId = if (_jipId isNotEqualTo "") then {
            _jipId;
        }
        else {
            if (_specialIdOverride isNotEqualTo "") then {
                _specialIdOverride;
            }
            else {
                generateUid;
            };
        };

        missionNamespace setVariable [_jipId, true, 2];

        [
            "KH_eve_execution", 
            [_arguments, _function, clientOwner, _unscheduled],
            _target, 
            [_dependency, _unitRequired, _jipId]
        ] call KH_fnc_triggerCbaEvent;
    };

    case "CALLBACK": {
        private _callbackArguments = _special param [1, []];
        private _callbackFunction = _special param [2, {}, ["", {}]];
        private _callbackId = generateUid;

        [
            "CBA",
            _callbackId,
            [_arguments, _function, _unscheduled],
            {
                _args params ["_arguments", "_function", "_unscheduled"];
                private _argsCallback = _this;

                if _unscheduled then {
                    if (isNil "_arguments") then {
                        call (missionNamespace getVariable _function);
                    }
                    else {
                        _arguments call (missionNamespace getVariable _function);
                    };
                }
                else {
                    if (isNil "_arguments") then {
                        [] spawn (missionNamespace getVariable _function);
                    }
                    else {
                        _arguments spawn (missionNamespace getVariable _function);
                    };
                };

                [_handlerId] call KH_fnc_removeHandler;
            }
        ] call KH_fnc_addEventHandler;

        [
            "KH_eve_registerCallback", 
            [_callbackArguments, [_callbackFunction, false] call KH_fnc_serializeFunction, clientOwner, _unscheduled, _callbackId], 
            _target, 
            false
        ] call KH_fnc_triggerCbaEvent;
    };

    case "PERSISTENT": {
        private _entity = _special param [1, objNull, [objNull, grpNull]];
        private _sendoffArguments = _special param [2];
        private _sendoffFunction = _special param [3, {}, ["", {}]];
        private _persistentExecutionId = _special param [4, "", [""]];

        _persistentExecutionId = if (_persistentExecutionId isNotEqualTo "") then {
            _persistentExecutionId;
        }
        else {
            if (_specialIdOverride isNotEqualTo "") then {
                _specialIdOverride;
            }
            else {
                generateUid;
            };
        };

        _entity setVariable [_persistentExecutionId, true, true];
        ["KH_eve_execution", [[_entity], {(_this select 0) setVariable ["KH_var_initialPersistencyCall", true];}, clientOwner, _unscheduled], _target, false] call KH_fnc_triggerCbaEvent;
        ["KH_eve_execution", [_arguments, _function, clientOwner, _unscheduled], _target, false] call KH_fnc_triggerCbaEvent;

        [
            "KH_eve_persistentExecutionSetup", 
            [_arguments, _function, _entity, _sendoffArguments, [_sendoffFunction, false] call KH_fnc_serializeFunction, clientOwner, _unscheduled, _persistentExecutionId],
            "SERVER",
            false
        ] call KH_fnc_triggerCbaEvent;

        [_entity, _persistentExecutionId, true];
    };

    case "PLAYER_PRESENCE": {
        private _object = _special param [1, objNull, [objNull]];
        private _present = _special param [2, true, [true]];
        private _distance = _special param [3, 0, [0]];
        private _jip = _special param [4, true, [true]];
        private _nearId = _special param [5, "", [""]];
        
        _nearId = if (_nearId isNotEqualTo "") then {
            _nearId;
        }
        else {
            if (_specialIdOverride isNotEqualTo "") then {
                _specialIdOverride;
            }
            else {
                generateUid;
            };
        };

        missionNamespace setVariable [_nearId, true, 2];
        ["KH_eve_execution", [_arguments, _function, clientOwner, _unscheduled], _target, false] call KH_fnc_triggerCbaEvent;
        ["KH_eve_playerPresenceExecutionSetup", [_arguments, _function, clientOwner, _unscheduled, _object, _present, _distance, _nearId, +KH_var_allPlayerControlledUnits, _jip], "SERVER", false] call KH_fnc_triggerCbaEvent;
        [missionNamespace, _nearId, 2];
    };

    default {
        nil;
    };
};