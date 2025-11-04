isNil {
	params [["_logic", objNull, [objNull]]];
	private _unit = attachedTo _logic;
    _unit setDamage 0;

    {
        _unit setHitPointDamage [_x, 0];
    } forEach ((getAllHitPointsDamage _unit) select 0);

    _unit setUnconscious false;

    [
        [_unit],
        {
            params ["_unit"];
            
            if (_unit isEqualTo player) then {
                if (player getVariable ["KH_var_withstanding", false]) then {
                    ppEffectDestroy [KH_var_withstandingEffectRadial, KH_var_withstandingEffectChromatic, KH_var_withstandingEffectWet];
                };

                ppEffectDestroy KH_var_incapacitationFade;
            };
        },
        _unit,
        true,
        false
    ] call KH_fnc_execute;

    _unit setVariable ["KH_var_incapacitated", false, true];
    _unit setVariable ["KH_var_withstanding", false, true];
    _unit setVariable ["KH_var_stabilized", false, true];
	deleteVehicle _logic;
};

nil;