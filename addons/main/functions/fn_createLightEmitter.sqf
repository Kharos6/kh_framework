params [
    ["_source", [0, 0, 0], [[], objNull]], 
    ["_light", "", [""]],
    ["_duration", 0, [0]],
    ["_conditionArguments", []],
    ["_conditionFunction", {true;}, [{}]], 
    ["_target", true, [true, 0, "", [], {}, objNull, teamMemberNull, grpNull, sideUnknown, locationNull]],
    ["_jip", false, [true, []]]
];

if (_duration isEqualTo 0) then {
    _duration = 999999;
};

private _lightId = generateUid;
missionNamespace setVariable [_lightId, true, true];

[
    [
        [_source, _light, _duration, _conditionArguments, _conditionFunction, _lightId],
        {
            params ["_source", "_light", "_duration", "_conditionArguments", "_conditionFunction", "_lightId"];
            if (!(missionNamespace getVariable _lightId) || !(_conditionArguments call _conditionFunction)) exitWith {};
            
            private _emitter = if (isClass (configFile >> "CfgLights" >> _particle)) then {
                private _startingPosition = _source;
                private _entity = objNull;
                private _selection = "";
                private _trackingPosition = [0, 0, 0];

                if (_source isEqualType []) then {
                    if !(_source isEqualTypeAll 0) then {
                        _entity = _source param [0, objNull, [objNull]];
                        _selection = _source param [1, "", [""]];
                        _trackingPosition = _source param [2, [], [[]]];
                        _startingPosition = ASLToATL (_entity modelToWorldVisualWorld (_entity selectionPosition _selection));
                    }; 
                }
                else {
                    _entity = _source;
                    _startingPosition = getPosATLVisual _source;
                };

                private _parameters = [configFile >> "CfgLights" >> (getText (_x >> "type"))] call KH_fnc_getLightParameters;
                private _lightSource = createVehicleLocal ["#lightpoint", _startingPosition, [], 0, "CAN_COLLIDE"];
                _lightSource setLightColor (_parameters select 0);
                _lightSource setLightAmbient ((_parameters select 1) select [0, 3]);
                _lightSource setLightIntensity (_parameters select 2);
                _lightSource setLightDayLight (_parameters select 3);
                _lightSource setLightUseFlare (_parameters select 4);
                _lightSource setLightFlareSize (_parameters select 5);
                _lightSource setLightFlareMaxDistance (_parameters select 6);
                _lightSource setLightIR (_parameters select 7);
                _lightSource setLightAttenuation (_parameters select 8);

                if !(isNull _entity) then {
                    _lightSource attachTo [_entity, _trackingPosition, _selection, true];
                };
            }
            else {
                objNull;
            };

            [
                [_conditionArguments, _conditionFunction, _emitter, diag_tickTime + _duration, _lightId],
                {
                    params ["_conditionArguments", "_conditionFunction", "_emitter", "_timeout", "_lightId"];

                    if (!(missionNamespace getVariable _lightId) || !(_conditionArguments call _conditionFunction) || (diag_tickTime > _timeout) || (isNull _emitter)) then {
                        deleteVehicle _emitter;
                        [_handlerId] call KH_fnc_removeHandler;
                    };
                },
                true,
                0,
                false
            ] call KH_fnc_execute;

            nil;
        },
        _target,
        true,
        _jip
    ] call KH_fnc_execute,
    [missionNamespace, _lightId, true]
];