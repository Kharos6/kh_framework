params [["_special", false, [true, [], createHashMap]], ["_target", true, [true, 0, "", [], {}, objNull, teamMemberNull, grpNull, sideUnknown, locationNull]]];

if ((_special isNotEqualTo false) && !(_special isEqualType createHashMap)) then {
    private _specialType = _special param [0, "", [""]];
    private _specialIdOverride = "";

    switch _specialType do {
        case "JIP": {
            private _specialId = _special param [3, "", [""]];

            if (_specialId isNotEqualTo "") then {
                _specialIdOverride = _specialId;
            }
            else {
                _specialIdOverride = generateUid;
            };

            [[missionNamespace, _specialIdOverride, 2], _specialIdOverride];
        };

        case "PLAYER_PRESENCE": {
            private _specialId = _special param [5, "", [""]];

            if (_specialId isNotEqualTo "") then {
                _specialIdOverride = _specialId;
            }
            else {
                _specialIdOverride = generateUid;
            };

            [[missionNamespace, _specialIdOverride, 2], _specialIdOverride];
        };

        case "PERSISTENT": {
            private _specialId = _special param [4, "", [""]];

            if (_specialId isNotEqualTo "") then {
                _specialIdOverride = _specialId;
            }
            else {
                _specialIdOverride = generateUid;
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