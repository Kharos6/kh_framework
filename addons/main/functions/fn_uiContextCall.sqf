params ["_arguments", ["_function", {}, [{}]]];

if hasInterface then {
    private _id = generateUid;

    if isUIContext then {
        execute [
            [_arguments, _function, _id],
            {
                params ["_arguments", "_function", "_id"];
                KH_var_uiContextExecutionStack pushBack [_arguments, _function, _id];
            },
            true,
            "-1",
            false
        ];
    }
    else {
        KH_var_uiContextExecutionStack pushBack [_arguments, _function, _id];
    };

    _id;
}
else {
    if (isNil "_arguments") then {
        call _function;
    }
    else {
        _arguments call _function;
    };
};