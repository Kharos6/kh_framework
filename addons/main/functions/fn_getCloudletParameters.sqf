params [["_cloudlet", "", ["", configNull]]];

private _currentEntry = if (_cloudlet isEqualType "") then {
    private _name = _cloudlet;
    _cloudlet = configFile >> "CfgCloudlets" >> _cloudlet;
    _name;
}
else {
    configName _cloudlet;
};

if !(KH_var_cloudletParameterCache isNil _currentEntry) exitWith {
    +(KH_var_cloudletParameterCache get _currentEntry);
};

private _result = [
    [
        [
            getText (_cloudlet >> "particleShape"),
            getNumber (_cloudlet >> "particleFSNtieth"),
            getNumber (_cloudlet >> "particleFSIndex"),
            getNumber (_cloudlet >> "particleFSFrameCount"),
            getNumber (_cloudlet >> "particleFSLoop")
        ],
        "",
        getText (_cloudlet >> "particleType"),
        ["SCALAR", _cloudlet >> "timerPeriod", 1] call KH_fnc_getConfigValue,
        ["SCALAR", _cloudlet >> "lifeTime", 1] call KH_fnc_getConfigValue,
        if ((getArray (_cloudlet >> "position")) isEqualTypeAll 0) then {
            getArray (_cloudlet >> "position");
        }
        else {
            [0, 0, 0];
        },
        if ((getArray (_cloudlet >> "moveVelocity")) isEqualTypeAll 0) then {
            getArray (_cloudlet >> "moveVelocity");
        }
        else {
            [0, 0, 0];
        },
        ["SCALAR", _cloudlet >> "rotationVelocity", 0] call KH_fnc_getConfigValue,
        ["SCALAR", _cloudlet >> "weight", 1] call KH_fnc_getConfigValue,
        ["SCALAR", _cloudlet >> "volume", 1] call KH_fnc_getConfigValue,
        ["SCALAR", _cloudlet >> "rubbing", 0.05] call KH_fnc_getConfigValue,
        if ((getArray (_cloudlet >> "size")) isEqualTypeAll 0) then {
            getArray (_cloudlet >> "size");
        }
        else {
            [1, 1];
        },
        if ((getArray (_cloudlet >> "color")) isEqualTypeAll []) then {
            (getArray (_cloudlet >> "color")) apply {
                [[1, 1, 1, 1], _x] select (_x isEqualTypeAll 0);
            };
        }
        else {
            [[1, 1, 1, 1]];
        },
        if ((getArray (_cloudlet >> "animationSpeed")) isEqualTypeAll 0) then {
            getArray (_cloudlet >> "animationSpeed");
        }
        else {
            [0];
        },
        ["SCALAR", _cloudlet >> "randomDirectionPeriod", 0] call KH_fnc_getConfigValue,
        ["SCALAR", _cloudlet >> "randomDirectionIntensity", 0] call KH_fnc_getConfigValue,
        getText (_cloudlet >> "onTimerScript"),
        getText (_cloudlet >> "beforeDestroyScript"),
        objNull,
        ["SCALAR", _cloudlet >> "angle", 0] call KH_fnc_getConfigValue,
        true,
        -1,
        [[0, 0, 0, 0]],
        [[0, 1, 0], [0, 0, 1]]
    ],
    [
        ["SCALAR", _cloudlet >> "lifeTimeVar", 0] call KH_fnc_getConfigValue,
        if ((getArray (_cloudlet >> "positionVar")) isEqualTypeAll 0) then {
            getArray (_cloudlet >> "positionVar");
        }
        else {
            [0, 0, 0];
        },
        if ((getArray (_cloudlet >> "moveVelocityVar")) isEqualTypeAll 0) then {
            getArray (_cloudlet >> "moveVelocityVar");
        }
        else {
            [0, 0, 0];
        },
        ["SCALAR", _cloudlet >> "rotationVelocityVar", 0] call KH_fnc_getConfigValue,
        ["SCALAR", _cloudlet >> "sizeVar", 0] call KH_fnc_getConfigValue,
        if ((getArray (_cloudlet >> "colorVar")) isEqualTypeAll 0) then {
            getArray (_cloudlet >> "colorVar");
        }
        else {
            [1, 1, 1, 1];
        },
        ["SCALAR", _cloudlet >> "randomDirectionPeriodVar", 0] call KH_fnc_getConfigValue,
        ["SCALAR", _cloudlet >> "randomDirectionIntensityVar", 0] call KH_fnc_getConfigValue,
        ["SCALAR", _cloudlet >> "angleVar", 0] call KH_fnc_getConfigValue,
        0
    ],
    [
        ["SCALAR", _cloudlet >> "coreIntensity", 0] call KH_fnc_getConfigValue,
        ["SCALAR", _cloudlet >> "coreDistance", 0] call KH_fnc_getConfigValue,
        ["SCALAR", _cloudlet >> "damageTime", 0] call KH_fnc_getConfigValue
    ],
    ["SCALAR", _cloudlet >> "interval", 0.1] call KH_fnc_getConfigValue
];

KH_var_cloudletParameterCache set [_currentEntry, _result];
+_result;