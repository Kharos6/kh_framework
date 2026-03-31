private _forward = (inputAction "MoveForward") isNotEqualTo 0;
private _backward = (inputAction "MoveBack") isNotEqualTo 0;
private _leftward = (inputAction "TurnLeft") isNotEqualTo 0;
private _rightward = (inputAction "TurnRight") isNotEqualTo 0;
private _run = ((inputAction "turbo") isNotEqualTo 0) && !(isForcedWalk KH_var_playerUnit);

[
    if _forward then {
        if _leftward then {
            "FORWARD_LEFTWARD";
        }
        else {
            if _rightward then {
                "FORWARD_RIGHTWARD";
            }
            else {
                "FORWARD";
            };
        };
    }
    else {
        if _backward then {
            if _leftward then {
                "BACKWARD_LEFTWARD";
            }
            else {
                if _rightward then {
                    "BACKWARD_RIGHTWARD";
                }
                else {
                    "BACKWARD";
                };
            };
        }
        else {
            if _leftward then {
                "LEFTWARD";
            }
            else {
                if _rightward then {
                    "RIGHTWARD";
                }
                else {
                    "";
                };
            };
        };
    },
    _run,
    (isForcedWalk KH_var_playerUnit) || (!_run && (isWalking KH_var_playerUnit))
];