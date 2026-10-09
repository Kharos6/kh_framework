params [["_unit", objNull, [objNull]]];
private _vehicle = objectParent _unit;

if (isNull _vehicle) exitWith {
    [];
};

if (_unit isEqualTo (driver _vehicle)) then {
    [_vehicle, "DRIVER"];
}
else {
    if (_unit isEqualTo (commander _vehicle)) then {
        [_vehicle, "COMMANDER"];
    }
    else {
        if (_unit isEqualTo (gunner _vehicle)) then {
            [_vehicle, "GUNNER"];
        }
        else {
            private _return = [_vehicle, "ANY"];

            {
                if (_unit isEqualTo (_vehicle turretUnit _x)) then {
                    _return = [_vehicle, "TURRET", _x];
                    break;
                };
            } forEach (allTurrets _vehicle);

            if ((_return select 1) isEqualTo "ANY") then {
                _return = [_vehicle, "CARGO", _vehicle getCargoIndex _unit];
            };

            _return;
        };
    };
};