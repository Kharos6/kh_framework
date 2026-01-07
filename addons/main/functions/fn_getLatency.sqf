params [["_target", objNull, [objNull]]];
private _owner = _target getVariable ["KH_var_owner", 2];

switch _owner do {
    case clientOwner: {
        0;
    };

    case 2: {
        private _result = missionNamespace getVariable [["KH_var_ping_", clientOwner] joinString "", 0];

        if (_result isNotEqualTo 0) then {
            _result / 1000;
        }
        else {
            0;
        };
    };

    default {
        private _result = (missionNamespace getVariable [["KH_var_ping_", clientOwner] joinString "", 0]) + (missionNamespace getVariable [["KH_var_ping_", _owner] joinString "", 0]);

        if (_result isNotEqualTo 0) then {
            _result / 1000;
        }
        else {
            0;
        };
    };
};