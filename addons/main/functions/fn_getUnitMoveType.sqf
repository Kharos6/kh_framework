params [["_unit", objNull, [objNull]], ["_registerStop", true, [true]]];

if (_unit isEqualTo KH_var_playerUnit) then {
    if (_registerStop && (((["KH_STRING", [true, true, false]] call KH_fnc_getPlayerMoveInput) select 0) isEqualTo "")) then {
        "STOP";
    }
    else {
        if (isForcedWalk _unit) then {
            "WALK";
        }
        else {
            if (isWalking _unit) then {
                "WALK";
            }
            else {
                if ((inputAction "turbo") isNotEqualTo 0) then {
                    "FAST";
                }
                else {
                    if ((_unit combatPace []) isNotEqualTo "None") then {
                        "TACT";
                    }
                    else {
                        "SLOW";
                    };
                };
            };
        };
    };
}
else {
    if (_registerStop && ((vectorMagnitude (velocity _unit)) isEqualTo 0)) then {
        "STOP";
    }
    else {
        if ((isForcedWalk _unit) || ((speedMode _unit) isEqualTo "LIMITED")) then {
            "WALK";
        }
        else {
            if (isWalking _unit) then {
                "WALK";
            }
            else {
                if ((speedMode _unit) isEqualTo "FULL") then {
                    "FAST";
                }
                else {
                    if ((_unit combatPace []) isNotEqualTo "None") then {
                        "TACT";
                    }
                    else {
                        "SLOW";
                    };
                };
            };
        };
    };
};