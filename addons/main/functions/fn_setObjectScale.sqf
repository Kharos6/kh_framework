params [["_object", objNull, [objNull]], ["_scale", 1, [0]]];

[
    [_object, _scale],
    {
        params ["_object", "_scale"];
        [_object, KH_var_helperLogic, true] call BIS_fnc_attachToRelative;
        _object setObjectScale _scale;
    },
    _object,
    true,
    false
] call KH_fnc_execute;