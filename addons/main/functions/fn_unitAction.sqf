params [["_unit", objNull, [objNull]], ["_action", "", [""]], ["_subaction", "", [0, "", []]]];
if !(alive _unit) exitWith {};

switch _action do {
    case "BEHAVIOUR": {
        _unit setBehaviourStrong _subaction;
    };

    case "COMBAT_MODE": {
        _unit setUnitCombatMode _subaction;
    };

    case "STANCE": {
        _unit setUnitPos _subaction;
    };

    case "MOVE": {
        doStop _unit;

        switch (_subaction param [0, "", [""]]) do {
            case "DESTINATION": {
                _unit move (_subaction param [1, [], [[]]]);
            };

            case "STOP": {
                switch (stance _unit) do {
                    case "STAND": {
                        _unit playActionNow "Stand";
                    };

                    case "CROUCH": {
                        _unit playActionNow "Crouch";
                    };

                    case "PRONE": {
                        _unit playActionNow "Lying";
                    };

                    default {
                        _unit playActionNow "Default";
                    };
                };
            };

            default {
                _unit playActionNow (_subaction param [1, "", [""]]);
            };
        };
    };

    case "TURN": {
        _subaction = _subaction * diag_deltaTime;

        if ((vectorMagnitude (velocity _unit)) isEqualTo 0) then {
            _unit playActionNow (["Turn_L", "Turn_R"] select (_subaction > 0));
        };

        _unit setDir ((getDir _unit) + _subaction);
    };

    case "TASK": {
        switch (_subaction param [0, "", [""]]) do {
            case "NONE": {
                _unit doTarget objNull;
                _unit doFire objNull;
            };

            case "TARGET": {
                _unit doTarget (_subaction param [1, objNull, [objNull]]);
            };

            case "FIRE": {
                _unit doFire (_subaction param [1, objNull, [objNull]]);
            };

            case "SELECT_WEAPON": {
                _unit selectWeapon (_subaction param [1, [], []]);
            };

            case "ACTION": {
                _unit action (_subaction param [1, [], [[]]]);
            };
        };
    };
};

nil;