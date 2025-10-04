KH_var_gameSessionId = call KH_fnc_generateUid;
KH_var_missionSessionId = call KH_fnc_generateUid;
KH_var_serverGameSessionId = KH_var_gameSessionId;
KH_var_serverMissionSessionId = KH_var_missionSessionId;
KH_var_aceLoaded = isClass (configFile >> "CfgPatches" >> "ace_main");

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

        _name luaCompile _function;

		if (isNumber (_x >> "preStart")) then {
			if ((getNumber (_x >> "preStart")) isEqualTo 1) then {
        		luaExecute _function;
			};
        };

		if (isNumber (_x >> "preInit")) then {
			if ((getNumber (_x >> "preInit")) isEqualTo 1) then {
        		KH_var_preInitLuaExecutions pushBack _function;
			};
        };

		if (isNumber (_x >> "postInit")) then {
			if ((getNumber (_x >> "postInit")) isEqualTo 1) then {
        		KH_var_postInitLuaExecutions pushBack _function;
			};
        };

		if (isNumber (_x >> "loadInit")) then {
			if ((getNumber (_x >> "loadInit")) isEqualTo 1) then {
        		KH_var_loadInitLuaExecutions pushBack _function;
			};
        };
    } forEach ("true" configClasses _config);
} forEach ("true" configClasses (configFile >> "CfgLuaFunctions"));

nil;