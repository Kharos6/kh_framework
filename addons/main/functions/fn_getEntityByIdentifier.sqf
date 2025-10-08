params [["_type", objNull, ["", objNull, grpNull, locationNull]], ["_identifier", "", [""]]];
private _entity = missionNamespace getVariable _identifier;

if (isNil "_entity") then {
    if (_type isNotEqualTo "") then {
        _type = typeName _type;
    };

    _entity = switch _type do {
        case "OBJECT": {
            private _allUnits = allUnits + allDeadMen;
            private _index = _allUnits findIf {_identifier isEqualTo (vehicleVarName _x);};
            
            if (_index isNotEqualTo -1) then {
                _allUnits select _index;
            }
            else {
                objNull;
            };
        };

        case "GROUP": {
            private _allGroups = allGroups;
            private _index = _allGroups findIf {_identifier isEqualTo (groupId _x);};

            if (_index isNotEqualTo -1) then {
                _allGroups select _index;
            }
            else {
                grpNull;
            };
        };

        case "LOCATION": {
            private _allLocations = nearestLocations [[worldSize / 2, worldSize / 2], [], worldSize * sqrt 2 / 2];
            private _index = _allLocations findIf {_identifier isEqualTo (className _x);};
            
            if (_index isNotEqualTo -1) then {
                _allLocations select _index;
            }
            else {
                locationNull;
            };
        };

        default {
            nil;
        };
    };
};

_entity;