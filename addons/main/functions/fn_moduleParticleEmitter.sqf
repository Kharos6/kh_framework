isNil {
    params [["_mode", "", [""]], ["_input", [], [[]]]];
    
    if is3DEN then {
        if (isNil "KH_var_allTiedEntities") then {
            KH_var_allTiedEntities = [];
        };

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
                _x params ["_entity", "_tiedEntity"];
                
                if (_entity isEqualTo _logic) then {
                    deleteVehicle _tiedEntity;
                };
            } forEach KH_var_allTiedEntities;
        };

        default {
            _input params [["_logic", objNull, [objNull]]];
            private _particle = ((_logic get3DENAttribute "KH_ModuleParticleEmitterParticle") select 0) select 0;
            private _position = getPosATL _logic;

            {
                _x params ["_entity", "_tiedEntity"];
                
                if (_entity isEqualTo _logic) then {
                    _tiedEntity setPosATL _position;
                };
            } forEach KH_var_allTiedEntities;

            if ((_logic getVariable ["KH_var_particleClass", ""]) isNotEqualTo _particle) then {
                _logic setVariable ["KH_var_particleClass", _particle];

                {
                    _x params ["_entity", "_tiedEntity"];
                    
                    if (_entity isEqualTo _logic) then {
                        deleteVehicle _tiedEntity;
                    };
                } forEach KH_var_allTiedEntities;

                if (isClass (configFile >> "CfgCloudlets" >> _particle)) then {
                    private _parameters = [configFile >> "CfgCloudlets" >> _particle] call KH_fnc_getCloudletParameters;
                    private _particleEffect = createVehicleLocal ["#particlesource", _position, [], 0, "CAN_COLLIDE"];
                    KH_var_allTiedEntities pushBack [_logic, _particleEffect];
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
                            KH_var_allTiedEntities pushBack [_logic, _particleEffect];
                            (_parameters select 0) set [18, _particleEffect];
                            _particleEffect setParticleParams (_parameters select 0);
                            _particleEffect setParticleRandom (_parameters select 1);
                            _particleEffect setParticleFire (_parameters select 2);
                            _particleEffect setDropInterval (_parameters select 3);
                        }
                        else {
                            private _parameters = [configFile >> "CfgLights" >> (getText (_x >> "type"))] call KH_fnc_getLightParameters;
                            private _light = createVehicleLocal ["#lightpoint", _position, [], 0, "CAN_COLLIDE"];
                            KH_var_allTiedEntities pushBack [_logic, _light];
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
            };
        };
    };
};

nil;