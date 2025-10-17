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
    } forEach ("true" configClasses _config);
} forEach (("true" configClasses (configFile >> "CfgLuaFunctions")) + ("true" configClasses (missionConfigFile >> "CfgLuaFunctions")));

nil;