params ["_callbackArguments", "_callbackFunction", "_id", "_owner"];
_callbackFunction = [_callbackFunction] call KH_fnc_parseFunction;
missionNamespace setVariable [_id, [_callbackArguments, _callbackFunction, clientOwner] call KH_fnc_callParsedFunction, _owner];