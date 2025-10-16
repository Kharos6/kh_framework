params [
    ["_text", "", [""]], 
    ["_resource", "", [""]], 
    ["_displayTimings", [1, 0, 1], [[]]], 
    ["_color", [0, 0, 0, 0], [[]]], 
    ["_transforms", [0, 0, 100, 100], [[]]], 
    ["_angle", [0, 0, 0], [[]]]
];

_displayTimings params [["_fadeIn", 1, [0]], ["_duration", 0, [0]], ["_fadeOut", 1, [0]]];
(_transforms call KH_fnc_parseNormalizedScreenTransforms) params ["_positionX", "_positionY", "_sizeX", "_sizeY"];
_angle params [["_angle", 0, [0]], ["_centerX", 0, [0]], ["_centerY", 0, [0]]];
private _khDisplay = uiNamespace getVariable ["KH_var_khDisplay", displayNull];
if (isNull _khDisplay) exitWith {};
private _control = _khDisplay ctrlCreate [_resource, -1];
_control ctrlSetFade 1;
_control ctrlSetPosition [_positionX, _positionY, _sizeX, _sizeY];
_control ctrlSetAngle [_angle, _centerX, _centerY];
_control ctrlCommit 0;

if (_text isEqualTo "") then {
    _control ctrlSetText "#(rgb,8,8,3)color(1,1,1,1)";
}
else {
    _control ctrlSetText _text;
};

_control ctrlSetTextColor _color;
_control ctrlSetFade 0;
_control ctrlCommit _fadeIn;

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