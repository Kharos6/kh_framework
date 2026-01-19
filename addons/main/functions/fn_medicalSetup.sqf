params [["_unit", objNull, [objNull]]];

if KH_var_medical then {
    [
        [_unit],
        {
            params ["_unit"];
            if (_unit getVariable ["KH_var_medicalInitComplete", false]) exitWith {};
            _unit setVariable ["KH_var_medicalInitComplete", true, true];
            KH_var_classDamageMultipliers = createHashMapFromArray (parseSimpleArray (["[", KH_var_classDamageMultipliersRaw, "]"] joinString ""));
            KH_var_childClassDamageMultipliers = parseSimpleArray (["[", KH_var_childClassDamageMultipliersRaw, "]"] joinString "");
            private _classDamageMultiplier = KH_var_classDamageMultipliers get (typeOf _unit);
            private "_childClassDamageMultiplier";
            
            {
                _x params ["_class", "_multiplier"];
                
                if (_unit isKindOf _class) then {
                    _childClassDamageMultiplier = _multiplier;
                    break;
                };
            } forEach KH_var_childClassDamageMultipliers;
            
            if !(isNil "_classDamageMultiplier") then {
                _unit setVariable ["KH_var_classDamageMultiplier", _classDamageMultiplier, true];
            };

            if !(isNil "_childClassDamageMultiplier") then {
                _unit setVariable ["KH_var_childClassDamageMultiplier", _childClassDamageMultiplier, true];
            };
            
            [_unit, "KH_var_stabilized", false, true, true] call KH_fnc_setRespawnVariable;
            [_unit, "KH_var_withstanding", false, true, true] call KH_fnc_setRespawnVariable;
            [_unit, "KH_var_incapacitated", false, true, true] call KH_fnc_setRespawnVariable;
            [_unit, "KH_var_beingTreated", false, true, true] call KH_fnc_setRespawnVariable;
            [_unit, "KH_var_beingRevived", false, true, true] call KH_fnc_setRespawnVariable;
            [_unit, "KH_var_beingStabilized", false, true, true] call KH_fnc_setRespawnVariable;
            [_unit, "KH_var_treating", false, true, true] call KH_fnc_setRespawnVariable;
            [_unit, "KH_var_reviving", false, true, true] call KH_fnc_setRespawnVariable;
            [_unit, "KH_var_stabilizing", false, true, true] call KH_fnc_setRespawnVariable;
            [_unit, "KH_var_unconsciousAnimationReset", false, true, true] call KH_fnc_setRespawnVariable;

            [
                ["ENTITY", _unit, "PERSISTENT"],
                "HandleDamage",
                [],
                {
                    params ["_unit", "_selection", "_damage", "_source", "_projectile", "_hitPartIndex", "_instigator", "_hitPoint", "_directHit", "_context"];
                    if !(_unit getVariable ["KH_var_medicalHandling", true]) exitWith {};
                    _hitPoint = toLowerANSI _hitPoint;

                    if ((_context isEqualTo 3) && (_hitPoint isEqualTo "hithead")) exitWith {
                        _unit getHitPointDamage "hithead";
                    };

                    if ((_context isEqualTo 0) && (_projectile isEqualTo "") && (isNull _source) && (isNull _instigator)) exitWith {
                        _damage;
                    };

                    private _totalDamage = false;

                    private _currentDamage = if ((_context isEqualTo 0) || (_context isEqualTo 4)) then {
                        _totalDamage = true;
                        damage _unit;
                    }
                    else {
                        _unit getHitPointDamage _hitPoint;
                    };

                    if (_projectile isEqualTo "") then {
                        _damage = _damage * KH_var_absoluteImpactDamageMultiplier;
                    };

                    private _hitPointType = switch _hitPoint do {
                        case "hithead";
                        case "hitface": {
                            "head";
                        };

                        case "hitneck": {
                            "neck";
                        };

                        case "hitpelvis";
                        case "hitabdomen": {
                            "pelvis";
                        };

                        case "hitdiaphragm";
                        case "hitchest": {
                            "torso";	
                        };

                        case "hithands";
                        case "hitarms": {
                            "arms";
                        };

                        case "hitlegs": {
                            "legs";
                        };
                        
                        default {
                            "body";
                        };
                    };
                    
                    private _isPlayer = isPlayer _unit;
                    private _incapacitated = _unit getVariable ["KH_var_incapacitated", false];
                    private _withstanding = _unit getVariable ["KH_var_withstanding", false];

                    private _processedDamage = (
                        _damage * 
                        (
                            switch _hitPointType do {
                                case "head": {
                                    KH_var_absoluteDamageMultiplierHead;
                                };

                                case "neck": {
                                    KH_var_absoluteDamageMultiplierNeck;
                                };

                                case "arm": {
                                    KH_var_absoluteDamageMultiplierArm;
                                };

                                case "leg": {
                                    KH_var_absoluteDamageMultiplierLeg;
                                };

                                case "torso": {
                                    KH_var_absoluteDamageMultiplierTorso;
                                };

                                case "pelvis": {
                                    KH_var_absoluteDamageMultiplierPelvis;
                                };
                                
                                default {
                                    1;
                                };
                            }
                        ) *
                        ([KH_var_absoluteAiDamageMultiplier, KH_var_absolutePlayerDamageMultiplier] select _isPlayer) * 
                        KH_var_absoluteDamageMultiplier *
                        (_unit getVariable ["KH_var_classDamageMultiplier", 1]) * 
                        (_unit getVariable ["KH_var_childClassDamageMultiplier", 1]) *
                        (_unit getVariable ["KH_var_damageMultiplier", 1])
                    );

                    if _totalDamage then {
                        if (
                            ((_currentDamage + (_processedDamage * KH_var_absoluteTotalDamageMultiplier)) >= KH_var_incapacitationThreshold) && 
                            (((_unit getVariable ["KH_var_allowIncapacitation", KH_var_allowIncapacitation]) && _isPlayer) || (_unit getVariable ["KH_var_allowIncapacitation", KH_var_allowAiIncapacitation]))
                           ) then {
                            if _withstanding then {
                                (
                                    _currentDamage + 
                                    (
                                        _processedDamage * 
                                        KH_var_absoluteWithstandingDamageMultiplier *
                                        KH_var_absoluteTotalDamageMultiplier
                                    )
                                ) min ([1, 0.99] select (_unit getVariable ["KH_var_plotArmor", false]));
                            }
                            else {
                                if _incapacitated then {
                                    (
                                        _currentDamage + 
                                        (
                                            _processedDamage * 
                                            KH_var_absoluteIncapacitatedDamageMultiplier *
                                            KH_var_absoluteTotalDamageMultiplier
                                        )
                                    ) min ([1, 0.99] select (_unit getVariable ["KH_var_plotArmor", false]));
                                }
                                else {
                                    _unit setUnconscious true;
                                    _unit setVariable ["KH_var_incapacitated", true, true];

                                    if KH_var_incapacitatedCaptives then {
                                        _unit setCaptive true;
                                    };

                                    if (_unit isEqualTo player) then {
                                        KH_var_incapacitationFade = ppEffectCreate ["ColorCorrections", 892003];
                                        KH_var_incapacitationFade ppEffectEnable true;
                                        KH_var_incapacitationFade ppEffectAdjust [1, 1, 0, [0, 0, 0, 0], [1, 1, 1, 0], [0.33, 0.33, 0.33, 0], [0, 0, 0, 0, 0, 0, 4]];
                                        KH_var_incapacitationFade ppEffectCommit KH_var_incapacitationDeathTime;
                                    };

                                    [
                                        [_unit, diag_tickTime + KH_var_incapacitationDeathTime],
                                        {
                                            params ["_unit", "_maximumTime"];

                                            if (diag_tickTime >= _maximumTime) then {
                                                if (_unit getVariable ["KH_var_incapacitated", false]) then {
                                                    _unit setDamage 1;
                                                };

                                                [_handlerId] call KH_fnc_removeHandler;
                                            }
                                            else {
                                                if !(_unit getVariable ["KH_var_incapacitated", false]) then {
                                                    [_handlerId] call KH_fnc_removeHandler;
                                                };
                                            };
                                        },
                                        true,
                                        0,
                                        false
                                    ] call KH_fnc_execute;

                                    if (_unit isEqualTo player) then {
                                        if KH_var_withstandingAllowed then {
                                            [
                                                [],
                                                {
                                                    if (!(player getVariable ["KH_var_incapacitated", false]) || !(alive player)) exitWith {};

                                                    [
                                                        [true, false],
                                                        [
                                                            "<img image='\A3\ui_f\data\igui\cfg\actions\bandage_ca.paa' size='1.8'/><br/>WITHSTAND",
                                                            nil,
                                                            nil,
                                                            "<img image='\A3\ui_f\data\igui\cfg\actions\bandage_ca.paa' size='1.8'/><br/>WITHSTANDING"
                                                        ],
                                                        [],
                                                        {
                                                            player setUnconscious false;

                                                            if KH_var_incapacitatedCaptives then {
                                                                player setCaptive false;
                                                            };

                                                            player setVariable ["KH_var_withstanding", true, true];
                                                            KH_var_withstandingEffectRadial = ppEffectCreate ["radialBlur", 892000];
                                                            KH_var_withstandingEffectRadial ppEffectEnable true;
                                                            KH_var_withstandingEffectRadial ppEffectAdjust [0.01, 0.01, 0.33, 0.33];
                                                            KH_var_withstandingEffectRadial ppEffectCommit 1;
                                                            KH_var_withstandingEffectChromatic = ppEffectCreate ["ChromAberration", 892001];
                                                            KH_var_withstandingEffectChromatic ppEffectEnable true;
                                                            KH_var_withstandingEffectChromatic ppEffectAdjust [0.005, 0.005, false];
                                                            KH_var_withstandingEffectChromatic ppEffectCommit 1;
                                                            KH_var_withstandingEffectWet = ppEffectCreate ["WetDistortion", 892002];
                                                            KH_var_withstandingEffectWet ppEffectEnable true;
                                                            KH_var_withstandingEffectWet ppEffectAdjust [1, 0.015, 0.015, 1, 1, 1, 1, 0.05, 0.01, 0.05, 0.01, 0.1, 0.1, 0.2, 0.2];
                                                            KH_var_withstandingEffectWet ppEffectCommit 1;

                                                            if (KH_var_withstandDuration isNotEqualTo 0) then {
                                                                [
                                                                    [diag_tickTime + KH_var_withstandDuration],
                                                                    {
                                                                        params ["_deadline"];

                                                                        if !(player getVariable ["KH_var_incapacitated", false]) then {
                                                                            [_handlerId] call KH_fnc_removeHandler;
                                                                        };

                                                                        if ((diag_tickTime >= _deadline) || !(alive player) || !(player getVariable ["KH_var_incapacitated", false])) then {
                                                                            if (player getVariable ["KH_var_withstanding", true]) then {
                                                                                if (player getVariable ["KH_var_incapacitated", false]) then {
                                                                                    player setUnconscious true;

                                                                                    if KH_var_incapacitatedCaptives then {
                                                                                        player setCaptive true;
                                                                                    };
                                                                                };

                                                                                player setVariable ["KH_var_withstanding", false, true];
                                                                            };

                                                                            [_handlerId] call KH_fnc_removeHandler;
                                                                        };
                                                                    },
                                                                    true,
                                                                    0,
                                                                    false
                                                                ] call KH_fnc_execute;
                                                            };

                                                            if KH_var_withstandingAllowCancel then {
                                                                [
                                                                    [true, false],
                                                                    "<img image='\A3\ui_f\data\igui\cfg\actions\bandage_ca.paa' size='1.8'/><br/>STOP WITHSTANDING",
                                                                    [],
                                                                    {
                                                                        player setUnconscious true;

                                                                        if KH_var_incapacitatedCaptives then {
                                                                            player setCaptive true;
                                                                        };
                                                                        
                                                                        player setVariable ["KH_var_withstanding", false, true];
                                                                    },
                                                                    [
                                                                        {
                                                                            ((player getVariable ["KH_var_withstanding", false]) && (player getVariable ["KH_var_incapacitated", false]));
                                                                        }
                                                                    ],
                                                                    false,
                                                                    true,
                                                                    true,
                                                                    [KH_var_withstandActionDuration, KH_var_withstandProgressDisplay],
                                                                    2,
                                                                    false,
                                                                    true,
                                                                    false,
                                                                    false,
                                                                    true,
                                                                    "",
                                                                    "",
                                                                    [true, false]
                                                                ] call KH_fnc_addAction;
                                                            };
                                                        },
                                                        [
                                                            {
                                                                ((player getVariable ["KH_var_incapacitated", false]) && (alive player));
                                                            },
                                                            {
                                                                ((!KH_var_withstandingRequireStabilization || (player getVariable ["KH_var_stabilized", false])) && (isNull (attachedTo player)));
                                                            }
                                                        ],
                                                        false,
                                                        true,
                                                        true,
                                                        [KH_var_withstandActionDuration, KH_var_withstandProgressDisplay],
                                                        2,
                                                        true,
                                                        true,
                                                        false,
                                                        false,
                                                        true,
                                                        "",
                                                        "",
                                                        [true, false]
                                                    ] call KH_fnc_addAction;
                                                },
                                                true,
                                                str KH_var_withstandDelay,
                                                false
                                            ] call KH_fnc_execute;
                                        };

                                        if KH_var_allowGivingUp then {
                                            [
                                                [true, false],
                                                [
                                                    "<img image='\A3\ui_f\data\igui\cfg\HoldActions\holdAction_forceRespawn_ca.paa' size='2.4'/><br/>GIVE UP",
                                                    nil,
                                                    nil,
                                                    "<img image='\A3\ui_f\data\igui\cfg\HoldActions\holdAction_forceRespawn_ca.paa' size='2.4'/><br/>GIVING UP"
                                                ],
                                                [],
                                                {
                                                    player setDamage 1;
                                                },
                                                [
                                                    {
                                                        ((player getVariable ["KH_var_incapacitated", false]) && (alive player));
                                                    }
                                                ],
                                                false,
                                                true,
                                                true,
                                                [3, KH_var_giveUpProgressDisplay],
                                                2,
                                                false,
                                                true,
                                                false,
                                                false,
                                                true,
                                                "",
                                                "",
                                                [true, false]
                                            ] call KH_fnc_addAction;
                                        };

                                        [
                                            [],
                                            {
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
                                            },
                                            true,
                                            {
                                                (!(player getVariable ["KH_var_incapacitated", false]) || !(alive player));
                                            },
                                            false
                                        ] call KH_fnc_execute;
                                    };

                                    [
                                        [_unit],
                                        {
                                            params ["_unit"];

                                            if (_unit getVariable ["KH_var_incapacitated", false]) then {
                                                if !(_unit getVariable ["KH_var_withstanding", false]) then {
                                                    if ((isNull (attachedTo _unit)) && (isNull (objectParent _unit)) && !("unconscious" in (animationState _unit)) && !("ainj" in (animationState _unit))) then {
                                                        [_unit, [["MOVE_SWITCH_GLOBAL", ["Unconscious"]]], false, false] call KH_fnc_setAnimation;
                                                        _unit setVariable ["KH_var_unconsciousAnimationReset", true, true];
                                                    };
                                                };
                                            }
                                            else {
                                                if ((isNull (objectParent _unit)) && (_unit getVariable ["KH_var_unconsciousAnimationReset", false])) then {
                                                    [_unit, [["MOVE_SWITCH_GLOBAL", ["AmovPpneMstpSnonWnonDnon"]]], false, false] call KH_fnc_setAnimation;
                                                };

                                                _unit setVariable ["KH_var_unconsciousAnimationReset", false, true];
                                                [_handlerId] call KH_fnc_removeHandler;
                                            };									
                                        },
                                        true,
                                        [
                                            0,
                                            false,
                                            false,
                                            {},
                                            false,
                                            true
                                        ],
                                        false
                                    ] call KH_fnc_execute;

                                    [
                                        [_unit, false],
                                        [
                                            ["<img image='\A3\ui_f\data\igui\cfg\HoldActions\holdAction_reviveMedic_ca.paa' size='2.4'/><br/>REVIVE ", toUpper (name _unit)] joinString "",
                                            nil,
                                            nil,
                                            ["<img image='\A3\ui_f\data\igui\cfg\HoldActions\holdAction_reviveMedic_ca.paa' size='2.4'/><br/>REVIVING ", toUpper (name _unit)] joinString ""
                                        ],
                                        [_unit],
                                        [
                                            {
                                                _target setVariable ["KH_var_beingRevived", true, true];
                                                _caller setVariable ["KH_var_reviving", true, true];

                                                if (isNull (objectParent _caller)) then {
                                                    if !("medic" in (animationState _caller)) then {
                                                        if (_caller isNotEqualTo _target) then {
                                                            _caller playActionNow "MedicStart";
                                                        }
                                                        else {
                                                            if (_caller getVariable ["KH_var_withstanding", false]) then {
                                                                _caller playActionNow "Medic";
                                                            };
                                                        };
                                                    };
                                                };

                                                _duration = [
                                                    [KH_var_reviveDuration, KH_var_selfReviveDuration] select (_caller isEqualTo _target), 
                                                    [KH_var_reviveMedicDuration, KH_var_selfReviveMedicDuration] select (_caller isEqualTo _target)
                                                ] select ([_caller getUnitTrait "Medic"] call KH_fnc_parseBoolean);

                                                nil;
                                            },
                                            {
                                                if (isNull (objectParent _caller)) then {
                                                    if !("medic" in (animationState _caller)) then {
                                                        if (_caller isNotEqualTo _target) then {
                                                            _caller playActionNow "MedicStart";
                                                        }
                                                        else {
                                                            if (_caller getVariable ["KH_var_withstanding", false]) then {
                                                                _caller playActionNow "Medic";
                                                            };
                                                        };
                                                    };
                                                };
                                            },
                                            {
                                                _target setVariable ["KH_var_beingRevived", false, true];
                                                _caller setVariable ["KH_var_reviving", false, true];

                                                if (!(_caller getVariable ["KH_var_incapacitated", false]) && (_caller isNotEqualTo _target)) then {
                                                    _caller switchAction "MedicStop";
                                                };
                                            },
                                            {
                                                _target setVariable ["KH_var_beingRevived", false, true];
                                                _caller setVariable ["KH_var_reviving", false, true];

                                                if (!(_caller getVariable ["KH_var_incapacitated", false]) && (_caller isNotEqualTo _target)) then {
                                                    _caller switchAction "MedicStop";
                                                };
                                            },
                                            {
                                                private _damageOffset = [
                                                    [KH_var_reviveHeal, KH_var_selfReviveHeal] select (_caller isEqualTo _target),
                                                    [KH_var_reviveHealMedic, KH_var_selfReviveHealMedic] select (_caller isEqualTo _target)
                                                ] select ([_caller getUnitTrait "Medic"] call KH_fnc_parseBoolean);

                                                _target setDamage (([(damage _target) - _damageOffset, 0] select (((damage _target) - _damageOffset) <= 0.25)) max 0);
                                                private _damages = getAllHitPointsDamage _target;

                                                if ((damage _target) isEqualTo 0) then {
                                                    _damageOffset = 1;
                                                };

                                                {
                                                    _target setHitPointDamage [_x, (((_damages select 2) select _forEachIndex) - _damageOffset) max 0];
                                                } forEach (_damages select 0);

                                                [
                                                    [_target, _caller],
                                                    {
                                                        params ["_target", "_caller"];
                                                        _target setUnconscious false;

                                                        if KH_var_incapacitatedCaptives then {
                                                            _target setCaptive false;
                                                        };
                                                    },
                                                    _target,
                                                    true,
                                                    false
                                                ] call KH_fnc_execute;

                                                if !("Medikit" in (items _caller)) then {
                                                    for "_i" from 1 to ([KH_var_reviveRequiredFirstAidKits, KH_var_selfReviveRequiredFirstAidKits] select (_caller isEqualTo _target)) do {
                                                        _caller removeItem "FirstAidKit";
                                                    };
                                                };

                                                _target setVariable ["KH_var_incapacitated", false, true];
                                                _target setVariable ["KH_var_withstanding", false, true];
                                                _target setVariable ["KH_var_stabilized", false, true];
                                                _target setVariable ["KH_var_beingRevived", false, true];
                                                _target setVariable ["KH_var_beingStabilized", false, true];
                                                _caller setVariable ["KH_var_reviving", false, true];

                                                if (_caller isNotEqualTo _target) then {
                                                    _caller switchAction "MedicStop";
                                                };
                                            },
                                            {
                                                params ["_unit"];
                                                _unit setVariable ["KH_var_incapacitated", false, true];
                                                _unit setVariable ["KH_var_withstanding", false, true];
                                                _unit setVariable ["KH_var_stabilized", false, true];
                                                _unit setVariable ["KH_var_beingRevived", false, true];
                                                _unit setVariable ["KH_var_beingStabilized", false, true];
                                            }
                                        ],
                                        [
                                            {
                                                params ["_unit"];
                                                ((_unit getVariable ["KH_var_incapacitated", false]) && (alive _unit));
                                            },
                                            {
                                                if ((_target getVariable ["KH_var_beingStabilized", false]) || (_caller getVariable ["KH_var_stabilizing", false]) || (_caller getVariable ["KH_var_treating", false])) exitWith {
                                                    false;
                                                };

                                                if (KH_var_reviveRequireStabilization && !(_target getVariable ["KH_var_stabilized", false])) exitWith {
                                                    false;
                                                };

                                                if (_caller isEqualTo _target) then {
                                                    if KH_var_selfRevive then {																	
                                                        (
                                                            (
                                                                ("Medikit" in (items _caller)) ||
                                                                (!KH_var_selfReviveRequireMedikit && (([items _caller, "FirstAidKit"] call KH_fnc_countArrayElements) >= KH_var_selfReviveRequiredFirstAidKits))
                                                            ) &&
                                                            (!KH_var_selfReviveMedicOnly || ([_caller getUnitTrait "Medic"] call KH_fnc_parseBoolean)) &&
                                                            (!KH_var_selfReviveRequireWithstanding || (_caller getVariable ["KH_var_withstanding", false]))
                                                        );
                                                    }
                                                    else {
                                                        false;
                                                    };
                                                }
                                                else {
                                                    if !(_caller getVariable ["KH_var_incapacitated", false]) then {																	
                                                        if ((_caller getVariable ["KH_var_withstanding", false]) && !KH_var_withstandingAllowRevivingOthers) then {
                                                            false;
                                                        }
                                                        else {
                                                            if KH_var_reviveRequireMedikit then {
                                                                (
                                                                    (!KH_var_reviveMedicOnly || ([_caller getUnitTrait "Medic"] call KH_fnc_parseBoolean)) && 
                                                                    ("Medikit" in (items _caller))
                                                                );
                                                            }
                                                            else {
                                                                (
                                                                    (!KH_var_reviveMedicOnly || ([_caller getUnitTrait "Medic"] call KH_fnc_parseBoolean)) &&
                                                                    (
                                                                        (([items _caller, "FirstAidKit"] call KH_fnc_countArrayElements) >= KH_var_reviveRequiredFirstAidKits) ||
                                                                        ("Medikit" in (items _caller))
                                                                    )
                                                                );
                                                            };
                                                        };
                                                    }
                                                    else {
                                                        false;
                                                    };
                                                };
                                            },
                                            {},
                                            {},
                                            {}
                                        ],
                                        false,
                                        true,
                                        "PLAYERS",
                                        [KH_var_reviveDuration, KH_var_reviveProgressDisplay],
                                        2,
                                        true,
                                        true,
                                        false,
                                        false,
                                        true,
                                        "",
                                        "",
                                        [false, true]
                                    ] call KH_fnc_addAction;

                                    [
                                        [_unit, false],
                                        [
                                            "<img image='\A3\ui_f\data\igui\cfg\actions\loadVehicle_ca.paa' size='1.8'/><br/>LOAD TO NEAREST VEHICLE",
                                            nil,
                                            nil,
                                            "<img image='\A3\ui_f\data\igui\cfg\actions\loadVehicle_ca.paa' size='1.8'/><br/>LOADING"
                                        ],
                                        [_unit],
                                        {
                                            [_target, [((_target nearEntities ["AllVehicles", 10]) select {!(_x isKindOf "Man");}) select 0, "CARGO", 0], true] call KH_fnc_setUnitVehicleSlot;
                                        },
                                        [
                                            {
                                                params ["_unit"];
                                                (((_unit getVariable ["KH_var_incapacitated", false])) && (alive _unit));
                                            },
                                            {
                                                ((((_target nearEntities ["AllVehicles", 10]) select {!(_x isKindOf "Man");}) isNotEqualTo []) && (isNull (objectParent _target)) && (_target isNotEqualTo _caller));
                                            },
                                            {},
                                            {},
                                            {}
                                        ],
                                        true,
                                        true,
                                        "PLAYERS",
                                        [3, "BAR"],
                                        2,
                                        false,
                                        false,
                                        false,
                                        false,
                                        true,
                                        "",
                                        "",
                                        [false, true]
                                    ] call KH_fnc_addAction;				
                                
                                    if KH_var_stabilizationAllowed then {
                                        [
                                            [_unit, false],
                                            [
                                                ["<img image='\A3\ui_f\data\igui\cfg\HoldActions\holdAction_revive_ca.paa' size='2.4'/><br/>STABILIZE ", toUpper (name _unit)] joinString "",
                                                nil,
                                                nil,
                                                ["<img image='\A3\ui_f\data\igui\cfg\HoldActions\holdAction_revive_ca.paa' size='2.4'/><br/>STABILIZING ", toUpper (name _unit)] joinString ""
                                            ],
                                            [_unit],
                                            [
                                                {
                                                    _target setVariable ["KH_var_beingStabilized", true, true];
                                                    _caller setVariable ["KH_var_stabilizing", true, true];

                                                    if (isNull (objectParent _caller)) then {
                                                        if !("medic" in (animationState _caller)) then {
                                                            if (_caller isNotEqualTo _target) then {
                                                                _caller playActionNow "MedicOther";
                                                            }
                                                            else {
                                                                if (_caller getVariable ["KH_var_withstanding", false]) then {
                                                                    _caller playActionNow "Medic";
                                                                };
                                                            };
                                                        };
                                                    };

                                                    _duration = [
                                                        [KH_var_stabilizationDuration, KH_var_stabilizationDurationMedic] select ([_caller getUnitTrait "Medic"] call KH_fnc_parseBoolean),
                                                        [KH_var_selfStabilizationDuration, KH_var_selfStabilizationDurationMedic] select ([_caller getUnitTrait "Medic"] call KH_fnc_parseBoolean)
                                                    ] select (_caller isEqualTo _target);

                                                    nil;
                                                },
                                                {
                                                    if (isNull (objectParent _caller)) then {
                                                        if !("medic" in (animationState _caller)) then {
                                                            if (_caller isNotEqualTo _target) then {
                                                                _caller playActionNow "MedicOther";
                                                            }
                                                            else {
                                                                if (_caller getVariable ["KH_var_withstanding", false]) then {
                                                                    _caller playActionNow "Medic";
                                                                };
                                                            };
                                                        };
                                                    };
                                                },
                                                {
                                                    _target setVariable ["KH_var_beingStabilized", false, true];
                                                    _caller setVariable ["KH_var_stabilizing", false, true];

                                                    if (!(_caller getVariable ["KH_var_incapacitated", false]) && (_caller isNotEqualTo _target)) then {
                                                        _caller switchAction "MedicStop";
                                                    };
                                                },
                                                {
                                                    _target setVariable ["KH_var_beingStabilized", false, true];
                                                    _caller setVariable ["KH_var_stabilizing", false, true];

                                                    if (!(_caller getVariable ["KH_var_incapacitated", false]) && (_caller isNotEqualTo _target)) then {
                                                        _caller switchAction "MedicStop";
                                                    };
                                                },
                                                {
                                                    if !("Medikit" in (items _caller)) then {
                                                        for "_i" from 1 to KH_var_stabilizationRequiredFirstAidKits do {
                                                            _caller removeItem "FirstAidKit";
                                                        };
                                                    };
                                                    
                                                    _target setVariable ["KH_var_beingStabilized", false, true];
                                                    _target setVariable ["KH_var_stabilized", true, true];
                                                    _caller setVariable ["KH_var_stabilizing", false, true];

                                                    if (_caller isNotEqualTo _target) then {
                                                        _caller switchAction "MedicStop";
                                                    };
                                                },
                                                {}
                                            ],
                                            [
                                                {
                                                    params ["_unit"];
                                                    ((_unit getVariable ["KH_var_incapacitated", false]) && (alive _unit));
                                                },
                                                {
                                                    if ((_target getVariable ["KH_var_beingRevived", false]) || (_caller getVariable ["KH_var_reviving", false]) || (_caller getVariable ["KH_var_treating", false])) exitWith {
                                                        false;
                                                    };
                                                    
                                                    if (_caller isEqualTo _target) then {
                                                        if KH_var_selfStabilization then {
                                                            if (KH_var_selfStabilizationMedicOnly && !([_caller getUnitTrait "Medic"] call KH_fnc_parseBoolean)) exitWith {
                                                                false;
                                                            };

                                                            if KH_var_selfStabilizationRequireMedikit then {
                                                                "Medikit" in (items _caller);
                                                            }
                                                            else {
                                                                ((([items _caller, "FirstAidKit"] call KH_fnc_countArrayElements) >= KH_var_selfStabilizationRequiredFirstAidKits) || ("Medikit" in (items _caller)));
                                                            };
                                                        }
                                                        else {
                                                            false;
                                                        };
                                                    }
                                                    else {
                                                        if (KH_var_stabilizationMedicOnly && !([_caller getUnitTrait "Medic"] call KH_fnc_parseBoolean)) exitWith {
                                                            false;
                                                        };

                                                        if KH_var_stabilizationRequireMedikit then {
                                                            "Medikit" in (items _caller);
                                                        }
                                                        else {
                                                            ((([items _caller, "FirstAidKit"] call KH_fnc_countArrayElements) >= KH_var_stabilizationRequiredFirstAidKits) || ("Medikit" in (items _caller)));
                                                        };
                                                    };								
                                                },
                                                {},
                                                {},
                                                {}
                                            ],
                                            false,
                                            true,
                                            "PLAYERS",
                                            [KH_var_stabilizationDuration, KH_var_stabilizationProgressDisplay],
                                            2,
                                            true,
                                            false,
                                            false,
                                            false,
                                            true,
                                            "",
                                            "",
                                            [false, true]
                                        ] call KH_fnc_addAction;
                                    };

                                    if KH_var_incapacitationDamageSpillover then {
                                        (
                                            KH_var_incapacitationThreshold + (((_currentDamage + (_processedDamage * KH_var_absoluteTotalDamageMultiplier)) - KH_var_incapacitationThreshold) * KH_var_absoluteIncapacitatedDamageMultiplier)
                                        ) min ([1, 0.99] select (_unit getVariable ["KH_var_plotArmor", false]));
                                    }
                                    else {
                                        [KH_var_incapacitationThreshold, 0.99] select (_unit getVariable ["KH_var_plotArmor", false]);
                                    };
                                };
                            };
                        }
                        else {
                            (_currentDamage + (_processedDamage * KH_var_absoluteTotalDamageMultiplier)) min ([1, 0.99] select (_unit getVariable ["KH_var_plotArmor", false]));
                        };
                    }
                    else {
                        (_currentDamage + _processedDamage) min ([[0.99, 1] select KH_var_allowHitPointMaximumDamage, 0.99] select (_unit getVariable ["KH_var_plotArmor", false]));
                    };
                }
            ] call KH_fnc_addEventHandler;

            [
                [_unit],
                {
                    params ["_unit"];
                    
                    _unit addEventHandler [
                        "HandleHeal", 
                        {
                            params ["_injured", "_healer", "_isMedic", "_atVehicle"];
                            
                            if (!(_injured getVariable ["KH_var_beingTreated", false]) && !(_healer getVariable ["KH_var_treating", false]) && !(_healer getVariable ["KH_var_reviving", false]) && !(_healer getVariable ["KH_var_stabilizing", false])) then {
                                private _isSelfHeal = _injured isEqualTo _healer;
                                _injured setVariable ["KH_var_beingTreated", true, true];
                                _healer setVariable ["KH_var_treating", true, true];

                                [
                                    [_injured, _healer, _isMedic, _atVehicle, _isSelfHeal, damage _injured],
                                    {
                                        params ["_injured", "_healer", "_isMedic", "_atVehicle", "_isSelfHeal", "_oldDamage"];

                                        if ((damage _injured) < _oldDamage) then {
                                            isNil {
                                                private _damageOffset = if !(isNull _atVehicle) then {
                                                    1;
                                                }
                                                else {
                                                    if KH_var_batchHeal then {
                                                        private _healItemCount = ([items _healer, "FirstAidKit"] call KH_fnc_countArrayElements) + 1;
                                                        private _currentDamageOffset = 0;

                                                        if !("Medikit" in (items _healer)) then {
                                                            for "_i" from 1 to _healItemCount do {
                                                                _currentDamageOffset = _currentDamageOffset + ([
                                                                    [
                                                                        [KH_var_firstAidKitHeal, KH_var_firstAidKitHealWithstanding] select (_injured getVariable ["KH_var_withstanding", false]), 
                                                                        [KH_var_firstAidKitSelfHeal, KH_var_firstAidKitSelfHealWithstanding] select (_healer getVariable ["KH_var_withstanding", false])
                                                                    ] select _isSelfHeal, 
                                                                    [
                                                                        [KH_var_firstAidKitHealMedic, KH_var_firstAidKitHealWithstandingMedic] select (_injured getVariable ["KH_var_withstanding", false]), 
                                                                        [KH_var_firstAidKitSelfHealMedic, KH_var_firstAidKitSelfHealWithstandingMedic] select (_healer getVariable ["KH_var_withstanding", false])
                                                                    ] select _isSelfHeal
                                                                ] select _isMedic);

                                                                if (_i isNotEqualTo 1) then {
                                                                    _healer removeItem "FirstAidKit";
                                                                };

                                                                if (_currentDamageOffset >= 1) then {
                                                                    break;
                                                                };
                                                            };
                                                        }
                                                        else {
                                                            _currentDamageOffset = 1;
                                                        };

                                                        _currentDamageOffset min 1;
                                                    }
                                                    else {
                                                        ([[KH_var_firstAidKitHeal, KH_var_firstAidKitSelfHeal] select _isSelfHeal, [KH_var_firstAidKitHealMedic, KH_var_firstAidKitSelfHealMedic] select _isSelfHeal] select _isMedic) min 1;
                                                    };
                                                };

                                                _injured setDamage (([_oldDamage - _damageOffset, 0] select ((_oldDamage - _damageOffset) <= 0.25)) max 0);
                                                private _damages = getAllHitPointsDamage _injured;

                                                if ((damage _injured) isEqualTo 0) then {
                                                    _damageOffset = 1;
                                                };

                                                {
                                                    _injured setHitPointDamage [_x, (((_damages select 2) select _forEachIndex) - _damageOffset) max 0];
                                                } forEach (_damages select 0);
                                            };
                                        };

                                        _injured setVariable ["KH_var_beingTreated", false, true];
                                        _healer setVariable ["KH_var_treating", false, true];
                                    },
                                    true,
                                    {
                                        private _injured = param [0];
                                        private _oldDamage = param [5];
                                        ((damage _injured) isNotEqualTo _oldDamage);
                                    },
                                    false
                                ] call KH_fnc_execute;

                                nil;
                            }
                            else {
                                true;
                            };
                        }
                    ];
                },
                "GLOBAL",
                true,
                false
            ] call KH_fnc_execute;

            [
                ["ENTITY", _unit, "PERSISTENT"],
                "Respawn",
                [],
                {
                    params ["_unit"];

                    if KH_var_incapacitatedCaptives then {
                        _unit setCaptive false;
                    };

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
                }
            ] call KH_fnc_addEventHandler;
        },
        true,
        "1",
        false
    ] call KH_fnc_execute;
};