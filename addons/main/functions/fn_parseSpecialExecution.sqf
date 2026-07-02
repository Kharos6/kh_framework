params [["_special", false, [true, []]], ["_target", true, [true, 0, "", [], {}, objNull, teamMemberNull, grpNull, sideUnknown, locationNull]]];

if (_special isEqualTo true) then {
	_special = ["JIP", true, false, ""];
};

if (_special isNotEqualTo false) then {
    switch (_special param [0, "", [""]]) do {
        case "JIP": {
            private _specialId = _special param [3, "", [""]];

            private _specialIdOverride = if (_specialId isNotEqualTo "") then {
                _specialId;
            }
            else {
                generateUid;
            };

            [[missionNamespace, _specialIdOverride, 2], _specialIdOverride];
        };

        case "PLAYER_PRESENCE": {
            private _specialId = _special param [5, "", [""]];

            private _specialIdOverride = if (_specialId isNotEqualTo "") then {
                _specialId;
            }
            else {
                generateUid;
            };

            [[missionNamespace, _specialIdOverride, 2], _specialIdOverride];
        };

        case "PERSISTENT": {
            private _specialId = _special param [4, "", [""]];

            private _specialIdOverride = if (_specialId isNotEqualTo "") then {
                _specialId;
            }
            else {
                generateUid;
            };

            [[_target, _specialIdOverride, true], _specialIdOverride];
        };

        default {
            [[], ""];
        };
    };
}
else {
    [[], ""];
};