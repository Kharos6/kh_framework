params [["_identifier", "", [""]], ["_type", objNull, ["", objNull, teamMemberNull, grpNull, locationNull]]];
private _entity = missionNamespace getVariable _identifier;

if (isNil "_entity") then {
    if (_type isNotEqualTo "") then {
        _type = typeName _type;
    };

    _entity = switch _type do {
        case "OBJECT": {
            private _allUnits = allUnits + allDeadMen;
            private _index = _allUnits findIf {((_identifier isEqualTo (vehicleVarName _x)) || (_identifier isEqualTo (hashValue _x)));};
            
            if (_index isNotEqualTo -1) then {
                _allUnits select _index;
            }
            else {
                objNull;
            };
        };

        case "TEAM_MEMBER": {
            private _allAgents = agents;
            private _index = _allAgents findIf {((_identifier isEqualTo (vehicleVarName (agent _x))) || (_identifier isEqualTo (hashValue (agent _x))));};
            
            if (_index isNotEqualTo -1) then {
                _allAgents select _index;
            }
            else {
                objNull;
            };
        };

        case "GROUP": {
            private _allGroups = allGroups;
            private _index = _allGroups findIf {((_identifier isEqualTo (groupId _x)) || (_identifier isEqualTo (hashValue _x)));};

            if (_index isNotEqualTo -1) then {
                _allGroups select _index;
            }
            else {
                grpNull;
            };
        };

        case "LOCATION": {
            private _allLocations = nearestLocations [[worldSize / 2, worldSize / 2], [], worldSize * sqrt 2 / 2];
            private _index = _allLocations findIf {((_identifier isEqualTo (name _x)) || (_identifier isEqualTo (className _x)));};
            
            if (_index isNotEqualTo -1) then {
                _allLocations select _index;
            }
            else {
                locationNull;
            };
        };

        case "MARKER": {
            private _allMapMarkers = allMapMarkers;
            private _index = _allMapMarkers findIf {_identifier isEqualTo _x;};
            
            if (_index isNotEqualTo -1) then {
                _allMapMarkers select _index;
            }
            else {
                "";
            };
        };
    };
};

_entity;