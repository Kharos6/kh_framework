isNil {
    params [["_mode", "", [""]], ["_input", [], [[]]]];
    
    if is3DEN then {
        if (isNil "KH_var_cloudletParameterCache") then {
            KH_var_cloudletParameterCache = createHashMap;
        };

        if (isNil "KH_var_lightParameterCache") then {
            KH_var_lightParameterCache = createHashMap;
        };
    };

    switch _mode do {
        case "init": {
            _input params [["_logic", objNull, [objNull]], ["_activated", true, [true]], ["_curatorPlaced", false, [true]]];
            if is3DEN exitWith {};

            if _activated then {
                [
                    _logic, 
                    (_logic getVariable ["KH_ModuleParticleEmitterParticle", []]) select 0,
                    parseNumber (_logic getVariable ["KH_ModuleParticleEmitterDuration", "0"]),
                    [],
                    [_logic],
                    compile (_logic getVariable ["KH_ModuleParticleEmitterCondition", "true;"]), 
                    "GLOBAL",
                    true
                ] call KH_fnc_createParticleEmitter;
            };
        };

        case "unregisteredFromWorld3DEN": {
            _input params [["_logic", objNull, [objNull]]];
            
            {
                deleteVehicle _x;
            } forEach (_logic getVariable ["KH_var_allTiedEntities", []]);
        };

        default {
            _input params [["_logic", objNull, [objNull]]];

            if (_logic isNil "KH_var_allTiedEntities") then {
                _logic setVariable ["KH_var_allTiedEntities", []];
            };

            private _show = (_logic get3DENAttribute "KH_ModuleParticleEmitterShow") select 0;

            if !_show exitWith {
                _logic setVariable ["KH_var_particleShow", _show];
                
                {
                    deleteVehicle _x;
                } forEach (_logic getVariable ["KH_var_allTiedEntities", []]);
            };

            private _particle = ((_logic get3DENAttribute "KH_ModuleParticleEmitterParticle") select 0) select 0;
            private _duration = parseNumber ((_logic get3DENAttribute "KH_ModuleParticleEmitterDuration") select 0);
            private _position = getPosATL _logic;

            {
                _x setPosATL _position;
            } forEach (_logic getVariable ["KH_var_allTiedEntities", []]);

            if (((_logic getVariable ["KH_var_particleClass", ""]) isNotEqualTo _particle) || (_duration isNotEqualTo (_logic getVariable ["KH_var_particleDuration", 0])) || (_show isNotEqualTo (_logic getVariable ["KH_var_particleShow", false]))) then {
                _logic setVariable ["KH_var_particleShow", _show];
                _logic setVariable ["KH_var_particleClass", _particle];
                _logic setVariable ["KH_var_particleDuration", _duration];

                {
                    deleteVehicle _x;
                } forEach (_logic getVariable ["KH_var_allTiedEntities", []]);

                if (isClass (configFile >> "CfgCloudlets" >> _particle)) then {
                    private _parameters = [configFile >> "CfgCloudlets" >> _particle] call KH_fnc_getCloudletParameters;
                    private _particleEffect = createVehicleLocal ["#particlesource", _position, [], 0, "CAN_COLLIDE"];
                    (_logic getVariable ["KH_var_allTiedEntities", []]) pushBack _particleEffect;
                    (_parameters select 0) set [18, _particleEffect];
                    _particleEffect setParticleParams (_parameters select 0);
                    _particleEffect setParticleRandom (_parameters select 1);
                    _particleEffect setParticleFire (_parameters select 2);
                    _particleEffect setDropInterval (_parameters select 3);
                }
                else {
                    {
                        if ((toLowerANSI (getText (_x >> "simulation"))) isEqualTo "particles") then {
                            private _parameters = [configFile >> "CfgCloudlets" >> (getText (_x >> "type"))] call KH_fnc_getCloudletParameters;
                            private _particleEffect = createVehicleLocal ["#particlesource", _position, [], 0, "CAN_COLLIDE"];
                            (_logic getVariable ["KH_var_allTiedEntities", []]) pushBack _particleEffect;
                            (_parameters select 0) set [18, _particleEffect];
                            _particleEffect setParticleParams (_parameters select 0);
                            _particleEffect setParticleRandom (_parameters select 1);
                            _particleEffect setParticleFire (_parameters select 2);
                            _particleEffect setDropInterval (_parameters select 3);
                        }
                        else {
                            private _parameters = [configFile >> "CfgLights" >> (getText (_x >> "type"))] call KH_fnc_getLightParameters;
                            private _light = createVehicleLocal ["#lightpoint", _position, [], 0, "CAN_COLLIDE"];
                            (_logic getVariable ["KH_var_allTiedEntities", []]) pushBack _light;
                            _light setLightColor (_parameters select 0);
                            _light setLightAmbient ((_parameters select 1) select [0, 3]);
                            _light setLightIntensity (_parameters select 2);
                            _light setLightDayLight (_parameters select 3);
                            _light setLightUseFlare (_parameters select 4);
                            _light setLightFlareSize (_parameters select 5);
                            _light setLightFlareMaxDistance (_parameters select 6);
                            _light setLightIR (_parameters select 7);
                            _light setLightAttenuation (_parameters select 8);
                        };
                    } forEach ("true" configClasses (configFile >> _particle));
                };

                if (_duration isNotEqualTo 0) then {
                    [_logic, _duration, +(_logic getVariable ["KH_var_allTiedEntities", []])] spawn {
                        params ["_logic", "_duration", "_entities"];
                        sleep _duration;

                        {
                            deleteVehicle _x;
                        } forEach _entities;
                    };
                };
            };
        };
    };
};

nil;