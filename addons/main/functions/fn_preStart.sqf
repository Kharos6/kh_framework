uiNamespace setVariable ["KH_var_gameSessionId", generateUid];
uiNamespace setVariable ["KH_var_missionSessionId", generateUid];
uiNamespace setVariable ["KH_var_serverGameSessionId", uiNamespace getVariable "KH_var_gameSessionId"];
uiNamespace setVariable ["KH_var_serverMissionSessionId", uiNamespace getVariable "KH_var_missionSessionId"];
uiNamespace setVariable ["KH_var_aceLoaded", isClass (configFile >> "CfgPatches" >> "ace_main")];
uiNamespace setVariable ["KH_var_preInitLuaExecutions", []];
uiNamespace setVariable ["KH_var_postInitLuaExecutions", []];
uiNamespace setVariable ["KH_var_loadInitLuaExecutions", []];
uiNamespace setVariable ["KH_var_resetInitLuaExecutions", []];
private _preInitLuaExecutions = uiNamespace getVariable "KH_var_preInitLuaExecutions";
private _postInitLuaExecutions = uiNamespace getVariable "KH_var_postInitLuaExecutions";
private _loadInitLuaExecutions = uiNamespace getVariable "KH_var_loadInitLuaExecutions";
private _resetInitLuaExecutions = uiNamespace getVariable "KH_var_resetInitLuaExecutions";

{
    private _config = _x;
    private _prefix = getText (_config >> "prefix");
    private _basePath = (getText (_config >> "path")) regexReplace ["(/)", "\\"];

    {
        private _pathUsed = isText (_x >> "path");

        private _function = preprocessFile ([
            _basePath,
            ["", "\"] select (_basePath isNotEqualTo ""),
            if _pathUsed then {
                (getText (_x >> "path")) regexReplace ["(/)", "\\"];
            }
            else {
                "";
            }, 
            configName _x, 
            ".lua"
        ] joinString "");

        private _name = if (isText (_x >> "name")) then {
            [_prefix, getText (_x >> "name")] joinString "_";
        }
        else {
           	[_prefix, configName _x] joinString "_";
        };

		if (isNumber (_x >> "compileSqf")) then {
			if ((getNumber (_x >> "compileSqf")) isEqualTo 1) then {
                uiNamespace setVariable [
                    [
                        _prefix,
                        "_fnc_",
                        if (isText (_x >> "name")) then {
                            getText (_x >> "name");
                        }
                        else {
                            configName _x;
                        }
                    ] joinString "",
                    compile (["_this luaExecute '", _name, "';"] joinString "") 
                ];
            };
        };

        _name luaCompile _function;

		if (isNumber (_x >> "preStart")) then {
			if ((getNumber (_x >> "preStart")) isEqualTo 1) then {
        		luaExecute _function;
			};
        };

		if (isNumber (_x >> "preInit")) then {
			if ((getNumber (_x >> "preInit")) isEqualTo 1) then {
                if !(_name in _preInitLuaExecutions) then {
        		    _preInitLuaExecutions pushBack _name;
                };
			};
        };

		if (isNumber (_x >> "postInit")) then {
			if ((getNumber (_x >> "postInit")) isEqualTo 1) then {
                if !(_name in _postInitLuaExecutions) then {
        		    _postInitLuaExecutions pushBack _name;
                };
			};
        };

		if (isNumber (_x >> "loadInit")) then {
			if ((getNumber (_x >> "loadInit")) isEqualTo 1) then {
                if !(_name in _loadInitLuaExecutions) then {
        		    _loadInitLuaExecutions pushBack _name;
                };
			};
        };

		if (isNumber (_x >> "resetInit")) then {
			if ((getNumber (_x >> "resetInit")) isEqualTo 1) then {
                if !(_name in _resetInitLuaExecutions) then {
        		    _resetInitLuaExecutions pushBack _name;
                };
			};
        };
    } forEach ("true" configClasses _config);
} forEach ("true" configClasses (configFile >> "CfgLuaFunctions"));

uiNamespace setVariable [
    "KH_var_defaultAiSystemPrompt",
    [
        "The term USER INSTRUCTIONS refers to any user message.\n",
        "The USER INSTRUCTIONS are within the context of a military sandbox video game called Arma 3.\n",
        "Your thought process, decision-making, and responses must fully align with your assigned role and all circumstantial factors that are implicitly or explicitly supported by the USER INSTRUCTIONS.\n",
        "You must carefully consider minute details, explicit requirements, implicit intent, contextual nuances, and desired outcomes implied by the USER INSTRUCTIONS.\n",
        "You must be creative while remaining consistent with the information provided by the USER INSTRUCTIONS; exercise innovations and unconventional solutions that are logically supported by the USER INSTRUCTIONS.\n",
        "If the USER INSTRUCTIONS are unclear or contradictory, you must interpret them in the way most consistent with your assigned role and current context.\n",
        "Your responses must be concise and contain only the level of verbosity that is necessary to satisfy the directive defined by the USER INSTRUCTIONS without being excessively long.\n",
        "Your responses must not contain your thought process, commentary, confirmations, or acknowledgements of any instructions or directives.\n",
        "\n",
        "You have extensive knowledge of essential concepts within Arma 3 as defined by the following information:\n",
        "All vectors with two or three numeric elements that are referenced as a position, rotation, velocity, or angular velocity are formatted as [X, Y, Z]; the X, Y, and Z axis elements can be negative or positive and are defined by the following rules:\n",
        "X AXIS: Negative is west or left, positive is east or right. For rotation, this axis uses absolute values from 0 to 360 degrees and is referenced as pitch.\n",
        "Y AXIS: Negative is south or backward, positive is north or forward. For rotation, this axis uses absolute values from 0 to 360 degrees and is referenced as bank.\n",
        "Z AXIS: Negative is down, positive is up. For rotation, this axis uses absolute values from 0 to 360 degrees and is referenced as yaw.\n",
        "POSITION: A vector that represents position in metres using the [X, Y, Z] or [X, Y] format, and can be relative to either the world or an explicitly specified position; the 5 possible position types that define the relativity of the Z height element are:\n",
        "ASL: Above sea level; this is the default if position type is undefined.\n",
        "ASLW: Above sea waves level.\n",
        "ATL: Above terrain level.\n",
        "AGL: Above ground level; the ground level being the sea level if over sea, and terrain level if over terrain.\n",
        "AGLS: Above nearest surface level.\n",
        "ROTATION: A vector that represents rotation in degrees using the euler [X, Y, Z] format from 0 to 360 degrees, and can be relative to either the world or an explicitly specified position.\n",
        "VELOCITY: A vector that represents positional speed in metres per second using the [X, Y, Z] format, and can be relative to either the world or an explicitly specified position.\n",
        "ANGULAR VELOCITY: A vector that represents rotational speed in degrees per second using the [X, Y, Z] format, and can be relative to either the world or an explicitly specified position.\n",
        "DIRECTION: An absolute value from 0 to 360 that represents compass direction in degrees, and can be relative to either the world or an explicitly specified position; 0 or 360 is North, 90 is East, 180 is South, and 270 is West.\n",
        "UNIT: An entity that is directly controlled by an AI or by a player.\n",
        "OBJECT: An environmental entity, such as a prop affected by physics or a static structure, that cannot be directly or indirectly controlled by an AI or by a player.\n",
        "VEHICLE: An entity similar to an object, but can be entered and indirectly controlled by one or multiple units.\n",
        "GROUP: An organized collection of units; groups may sometimes be empty, but units always belong to a group.\n",
        "SIDE: Represents the affiliation of units and groups; sides have relations that dictate whether units of different sides are friendly, neutral, or hostile towards each other.\n",
        "The overall measurement system used is the metric system, time is in the 24-hour format, dates are in the day/month/year format, and speed or velocity is measured in metres per second.\n"
    ] joinString ""
];

nil;