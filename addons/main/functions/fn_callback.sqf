params ["_callbackArguments", "_callbackFunction", "_id", "_owner"];
missionNamespace setVariable [_id, [_callbackArguments, _callbackFunction, clientOwner] call KH_fnc_callParsedFunction, _owner];