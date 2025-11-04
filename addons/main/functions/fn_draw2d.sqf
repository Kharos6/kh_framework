params [
    ["_resource", "RscText", [""]],
    ["_text", "", [""]], 
    ["_displayTimings", [1, 0, 1], [[]]], 
    ["_backgroundColor", [0, 0, 0, 1], [[]]], 
    ["_transforms", [0, 0, 100, 100], [[]]],
    ["_square", false, [true]], 
    ["_angle", [0, 0, 0], [[]]]
];

if (_resource isEqualTo "") then {
    _resource = "RscText";
};

_displayTimings params [["_fadeIn", 0.5, [0]], ["_duration", true, [true, 0]], ["_fadeOut", 0.5, [0]]];
_angle params [["_angle", 0, [0]], ["_centerX", 0, [0]], ["_centerY", 0, [0]]];
private _khDisplay = uiNamespace getVariable ["KH_var_khDisplay", displayNull];
private _control = _khDisplay ctrlCreate [_resource, -1];
_control ctrlSetFade 1;
_control ctrlSetPosition ([_transforms, _square] call KH_fnc_parseNormalizedScreenTransforms);
_control ctrlSetAngle [_angle, _centerX, _centerY, true];
_control ctrlCommit 0;

if (_text isEqualTo "") then {
    _control ctrlSetText (["#(rgb,8,8,3)color(", _backgroundColor select 0, ",", _backgroundColor select 1, ",", _backgroundColor select 2, ",", _backgroundColor select 3, ")"] joinString "");
}
else {
    _control ctrlSetText _text;
};

_control ctrlSetBackgroundColor _backgroundColor;
_control ctrlSetFade 0;
_control ctrlCommit _fadeIn;

if (_duration isEqualTo true) then {
    _duration = 0;
}
else {
    if (_duration isEqualTo false) then {
        _duration = -1;
    };
};

if (_duration >= 0) then {
    private _totalDuration = _duration + _fadeIn;

    [
        [_control, _fadeOut],
        {
            params ["_control", "_fadeOut"];
            _control ctrlSetFade 1;
            _control ctrlCommit _fadeOut;

            [
                [_control],
                {
                    params ["_control"];
                    ctrlDelete _control;
                },
                true,
                if (_fadeOut isNotEqualTo 0) then {
                    str _fadeOut;
                }
                else {
                    true;
                },
                false
            ] call KH_fnc_execute;
        },
        true,
        if (_totalDuration isNotEqualTo 0) then {
            str _totalDuration;
        }
        else {
            true;
        },
        false
    ] call KH_fnc_execute;
};

KH_var_allAddedDisplays pushBack _control;
_control;