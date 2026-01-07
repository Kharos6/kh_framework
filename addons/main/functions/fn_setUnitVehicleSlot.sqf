params [["_unit", objNull, [objNull]], ["_vehicle", [], [[], objNull]], ["_reassign", true, [true]]];

if (_vehicle isEqualType objNull) exitWith {
    if (isNull _vehicle) then {
        unassignVehicle _unit;
        moveOut _unit;
    }
    else {
        if ((_vehicle emptyPositions "") isNotEqualTo 0) then {
            _unit moveInAny _vehicle;
            true;
        }
        else {
            false;
        };
    };
};

_vehicle params [["_vehicle", objNull, [objNull]], ["_slot", "", [""]], ["_index", 0, [0, []]]];

if (isNull _vehicle) exitWith {
    unassignVehicle _unit;
    moveOut _unit;
};

if ((_vehicle emptyPositions "") isEqualTo 0) exitWith {
    false;
};

switch _slot do {
    case "DRIVER": {
        if (_reassign && !(isNull (driver _vehicle))) then {
            _unit moveInAny _vehicle;
        }
        else {
            _unit moveInDriver _vehicle;
        };
    };

    case "COMMANDER": {
        if (_reassign && !(isNull (commander _vehicle))) then {
            _unit moveInAny _vehicle;
        }
        else {
            _unit moveInCommander _vehicle;
        };
    };

    case "GUNNER": {
        if (_reassign && !(isNull (gunner _vehicle))) then {
            _unit moveInAny _vehicle;
        }
        else {
            _unit moveInGunner _vehicle;
        };
    };

    case "CARGO": {
        if (_reassign && ((_vehicle emptyPositions "Cargo") isEqualTo 0)) then {
            _unit moveInAny _vehicle;
        }
        else {
            _unit moveInCargo [_vehicle, _index, _reassign];
        };
    };

    case "TURRET": {
        if (_reassign && !(isNull (_vehicle turretUnit _index))) then {
            _unit moveInAny _vehicle;
        }
        else {
            _unit moveInTurret [_vehicle, _index];
        };
    };

    case "ANY": {
        _unit moveInAny _vehicle;
    };
};

true;