"kh_framework" callExtension ["GenerateRandomString", [1, true, false, false]];

if (isClass (configFile >> "CfgPatches" >> "ace_main")) then {
    uiNamespace setVariable ["KH_var_aceLoaded", true];
}
else {
    uiNamespace setVariable ["KH_var_aceLoaded", false];
};

true;