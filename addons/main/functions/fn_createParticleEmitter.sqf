params [
    ["_source", [0, 0, 0], [[], objNull]], 
    ["_particle", "", [""]],
    ["_duration", 0, [0]],
    ["_circleParameters", [], [[]]],
    ["_conditionArguments", []],
    ["_conditionFunction", {true;}, [{}]], 
    ["_target", true, [true, 0, "", [], {}, objNull, teamMemberNull, grpNull, sideUnknown, locationNull]],
    ["_jip", false, [true, []]]
];

if (_duration isEqualTo 0) then {
    _duration = 999999;
};

private _particleId = generateUid;
missionNamespace setVariable [_particleId, true, true];

[
    [
        [_source, _particle, _duration, _circleParameters, _conditionArguments, _conditionFunction, _particleId],
        {
            params ["_source", "_particle", "_duration", "_circleParameters", "_conditionArguments", "_conditionFunction", "_particleId"];
            if (!(missionNamespace getVariable _particleId) || !(_conditionArguments call _conditionFunction)) exitWith {};
            private _startingPosition = _source;
            private _entity = objNull;;
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

            private _particles = [];
            private _lights = [];

            if (isClass (configFile >> "CfgCloudlets" >> _particle)) then {
                private _parameters = [configFile >> "CfgCloudlets" >> _particle] call KH_fnc_getCloudletParameters;
                private _particleEffect = createVehicleLocal ["#particlesource", _startingPosition, [], 0, "CAN_COLLIDE"];
                _particles pushBack [_particleEffect, 999999];
                (_parameters select 0) set [18, _particleEffect];
                _particleEffect setParticleParams (_parameters select 0);
                _particleEffect setParticleRandom (_parameters select 1);
                _particleEffect setParticleFire (_parameters select 2);
                _particleEffect setDropInterval (_parameters select 3);

                if (_circleParameters isNotEqualTo []) then {
                    _particleEffect setParticleCircle _circleParameters;
                };

                if !(isNull _entity) then {
                    _particleEffect attachTo [_entity, _trackingPosition, _selection, true];
                };
            }
            else {
                if (isClass (configFile >> _particle)) then {
                    {
                        if ((toLowerANSI (getText (_x >> "simulation"))) isEqualTo "particles") then {
                            private _parameters = [configFile >> "CfgCloudlets" >> (getText (_x >> "type"))] call KH_fnc_getCloudletParameters;
                            private _particleEffect = createVehicleLocal ["#particlesource", _startingPosition, [], 0, "CAN_COLLIDE"];
                            _particles pushBack [_particleEffect, diag_tickTime + (getNumber (_x >> "lifeTime"))];
                            (_parameters select 0) set [18, _particleEffect];
                            _particleEffect setParticleParams (_parameters select 0);
                            _particleEffect setParticleRandom (_parameters select 1);
                            _particleEffect setParticleFire (_parameters select 2);
                            _particleEffect setDropInterval (_parameters select 3);

                            if (_circleParameters isNotEqualTo []) then {
                                _particleEffect setParticleCircle _circleParameters;
                            };

                            if !(isNull _entity) then {
                                _particleEffect attachTo [_entity, _trackingPosition, _selection, true];
                            };
                        }
                        else {
                            private _parameters = [configFile >> "CfgLights" >> (getText (_x >> "type"))] call KH_fnc_getLightParameters;
                            private _light = createVehicleLocal ["#lightpoint", _startingPosition, [], 0, "CAN_COLLIDE"];
                            _lights pushBack [_light, diag_tickTime + (getNumber (_x >> "lifeTime"))];
                            _light setLightColor (_parameters select 0);
                            _light setLightAmbient ((_parameters select 1) select [0, 3]);
                            _light setLightIntensity (_parameters select 2);
                            _light setLightDayLight (_parameters select 3);
                            _light setLightUseFlare (_parameters select 4);
                            _light setLightFlareSize (_parameters select 5);
                            _light setLightFlareMaxDistance (_parameters select 6);
                            _light setLightIR (_parameters select 7);
                            _light setLightAttenuation (_parameters select 8);

                            if !(isNull _entity) then {
                                _light attachTo [_entity, _trackingPosition, _selection, true];
                            };
                        };
                    } forEach ("true" configClasses (configFile >> _particle));
                };
            };

            [
                [_conditionArguments, _conditionFunction, _particleId, _particles, _lights, diag_tickTime + _duration],
                {
                    params ["_conditionArguments", "_conditionFunction", "_particleId", "_particles", "_lights", "_timeout"];

                    if (!(missionNamespace getVariable _particleId) || !(_conditionArguments call _conditionFunction) || (diag_tickTime > _timeout)) exitWith {
                        {
                            deleteVehicle (_x select 0);
                        } forEach (_particles + _lights);

                        [_handlerId] call KH_fnc_removeHandler;
                    };

                    private _particleDeletions = [];
                    private _lightDeletions = [];

                    {
                        if (diag_tickTime > (_x select 1)) then {
                            deleteVehicle (_x select 0);
                            _particleDeletions pushBack _forEachIndex;
                            continue;
                        };
                    } forEach _particles;

                    {
                        if (diag_tickTime > (_x select 1)) then {
                            deleteVehicle (_x select 0);
                            _lightDeletions pushBack _forEachIndex;
                            continue;
                        };
                    } forEach _lights;

                    _particles deleteAt _particleDeletions;
                    _lights deleteAt _lightDeletions;
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
    [missionNamespace, _particleId, true]
];