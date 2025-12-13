isNil {
	params [["_logic", objNull, [objNull]]];

	[
		[_logic],
		{
			params ["_logic"];
            private _unit = attachedTo _logic;
            _unit setDamage 0;

            {
                _unit setHitPointDamage [_x, 0];
            } forEach ((getAllHitPointsDamage _unit) select 0);

            [
                [_unit],
                {
                    params ["_unit"];
                    
                    if (_unit isEqualTo player) then {
                        if (!(isNil "KH_var_withstandingEffectRadial") && !(isNil "KH_var_withstandingEffectChromatic") && !(isNil "KH_var_withstandingEffectWet")) then {
                            ppEffectDestroy [KH_var_withstandingEffectRadial, KH_var_withstandingEffectChromatic, KH_var_withstandingEffectWet];
                            KH_var_withstandingEffectRadial = nil;
                            KH_var_withstandingEffectChromatic = nil;
                            KH_var_withstandingEffectWet = nil;
                        };
                        
                        if !(isNil "KH_var_incapacitationFade") then {
                            ppEffectDestroy KH_var_incapacitationFade;
                            KH_var_incapacitationFade = nil;
                        };	
                    };

                    _unit setUnconscious false;

                    if KH_var_medical then {
                        if KH_var_incapacitatedCaptives then {
                            _unit setCaptive false;
                        };
                    };
                },
                _unit,
                true,
                false
            ] call KH_fnc_execute;

            if KH_var_medical then {
                _unit setVariable ["KH_var_incapacitated", false, true];
                _unit setVariable ["KH_var_withstanding", false, true];
                _unit setVariable ["KH_var_stabilized", false, true];
                _unit setVariable ["KH_var_beingRevived", false, true];
                _unit setVariable ["KH_var_beingStabilized", false, true];
            };
            
            deleteVehicle _logic;
		},
		true,
		[
			{
				params ["_logic"];
				!(isNull (attachedTo _logic));
			},
			true,
			0,
			1,
			{},
			false,
			true
		],
		false
	] call KH_fnc_execute;
};

nil;