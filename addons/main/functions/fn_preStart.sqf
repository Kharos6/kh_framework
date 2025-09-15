KH_var_gameSessionId = call KH_fnc_generateUid;
KH_var_missionSessionId = call KH_fnc_generateUid;
KH_var_serverGameSessionId = KH_var_gameSessionId;
KH_var_serverMissionSessionId = KH_var_missionSessionId;
KH_var_aceLoaded = isClass (configFile >> "CfgPatches" >> "ace_main");
nil;