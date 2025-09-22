if (!call (uiNamespace getVariable ["INTERCEPT_BOOT_DONE",{false}])) then {
    #include "\x\kh\addons\intercept\boot.sqf"
    uiNamespace setVariable ['INTERCEPT_BOOT_DONE', compileFinal 'true'];
};

if (!isNil {_this}) then  {
    _this call compile preprocessFileLineNumbers "\A3\functions_f\initFunctions.sqf";
} else {
    call compile preprocessFileLineNumbers '\x\kh\addons\intercept\lib.sqf';
    call compile preprocessFileLineNumbers "\A3\functions_f\initFunctions.sqf";
};
