params [["_state", true]];

if _state then {
    KH_var_fixUnconsciousAnimationsState = true;

    if (isNil "KH_var_fixUnconsciousAnimationsSet") then {
        KH_var_fixUnconsciousAnimationsSet = true;

        [
            {
                if KH_var_fixUnconsciousAnimationsState then {
                    {
                        private _animation = animationState _x;
                        
                        if ((!(_x getVariable ["ACE_isUnconscious", false]) && !((lifeState _x) == "INCAPACITATED")) && (isNull (objectParent _x))) then {
                            if ((_animation == "unconscious") || (_animation == "deadstate") || ((_animation find "ace_medical_engine_uncon") != -1)) then {
                                [
                                    [_x],
                                    {
                                        params ["_unit"];
                                        _unit switchMove ["AmovPpneMstpSnonWnonDnon"];
                                    },
                                    "GLOBAL",
                                    "THIS_FRAME"
                                ] call KH_fnc_execute;
                            };
                        }; 
                    } forEach KH_var_allPlayerUnits;
                };
            }, 
            0.1, 
            []
        ] call CBA_fnc_addPerFrameHandler;
    };
}
else {
    KH_var_fixUnconsciousAnimationsState = false;
};

true;