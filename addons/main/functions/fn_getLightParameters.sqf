params [["_light", "", ["", configNull]]];

private _currentEntry = if (_light isEqualType "") then {
    private _name = _light;
    _light = configFile >> "CfgLights" >> _light;
    _name;
}
else {
    configName _light;
};

if !(KH_var_lightParameterCache isNil _currentEntry) exitWith {
    +(KH_var_lightParameterCache get _currentEntry);
};

private _attenuation = _light >> "Attenuation";

private _result = [
    if ((getArray (_light >> "color")) isEqualTypeAll 0) then {
        getArray (_light >> "color");
    }
    else {
        [1, 1, 1, 1];
    },
    if ((getArray (_light >> "ambient")) isEqualTypeAll 0) then {
        getArray (_light >> "ambient");
    }
    else {
        [1, 1, 1];
    },
    ["SCALAR", _light >> "intensity", 1] call KH_fnc_getConfigValue,
    (["SCALAR", _light >> "dayLight", 1] call KH_fnc_getConfigValue) isEqualTo 1,
    (["SCALAR", _light >> "useFlare", 1] call KH_fnc_getConfigValue) isEqualTo 1,
    ["SCALAR", _light >> "flareSize", 1] call KH_fnc_getConfigValue,
    ["SCALAR", _light >> "flareMaxDistance", 1] call KH_fnc_getConfigValue,
    (["SCALAR", _light >> "irLight", 0] call KH_fnc_getConfigValue) isEqualTo 1,
    [
        ["SCALAR", _attenuation >> "start", 1] call KH_fnc_getConfigValue,
        ["SCALAR", _attenuation >> "constant", 1] call KH_fnc_getConfigValue,
        ["SCALAR", _attenuation >> "linear", 1] call KH_fnc_getConfigValue,
        ["SCALAR", _attenuation >> "quadratic", 1] call KH_fnc_getConfigValue,
        ["SCALAR", _attenuation >> "hardLimitStart", 999999] call KH_fnc_getConfigValue,
        ["SCALAR", _attenuation >> "hardLimitEnd", 999999] call KH_fnc_getConfigValue
    ]
];

KH_var_lightParameterCache set [_currentEntry, _result];
+_result;