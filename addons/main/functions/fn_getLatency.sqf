params [["_target", objNull, [objNull, grpNull]], ["_roundTrip", true, [true]]];

if (_target isEqualType grpNull) then {
    _target = leader _target;
};

private _owner = _target getVariable ["KH_var_owner", 2];

switch _owner do {
    case clientOwner: {
        0;
    };

    case 2: {
        private _result = missionNamespace getVariable [["KH_var_ping_", clientOwner] joinString "", 0];

        if (_result isNotEqualTo 0) then {
            if _roundTrip then {
                _result / 1000;
            }
            else {
                (_result / 1000) / 2;
            };
        }
        else {
            0;
        };
    };

    default {
        private _result = (missionNamespace getVariable [["KH_var_ping_", clientOwner] joinString "", 0]) + (missionNamespace getVariable [["KH_var_ping_", _owner] joinString "", 0]);

        if (_result isNotEqualTo 0) then {
            if _roundTrip then {
                _result / 1000;
            }
            else {
                (_result / 1000) / 2;
            };
        }
        else {
            0;
        };
    };
};