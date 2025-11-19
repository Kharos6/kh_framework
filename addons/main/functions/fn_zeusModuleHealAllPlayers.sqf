isNil {
	params [["_logic", objNull, [objNull]]];

	{
        private _unit = _x;
        _unit setDamage 0;

        {
            _unit setHitPointDamage [_x, 0];
        } forEach ((getAllHitPointsDamage _unit) select 0);

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

                _unit setUnconscious false;

                if KH_var_incapacitatedCaptives then {
                    _unit setCaptive false;
                };
            },
            _unit,
            true,
            false
        ] call KH_fnc_execute;

        _unit setVariable ["KH_var_incapacitated", false, true];
        _unit setVariable ["KH_var_withstanding", false, true];
        _unit setVariable ["KH_var_stabilized", false, true];
        _unit setVariable ["KH_var_beingRevived", false, true];
        _unit setVariable ["KH_var_beingStabilized", false, true];
    } forEach KH_var_allPlayerUnits;

	deleteVehicle _logic;
};

nil;