params [["_type", objNull, ["", objNull, teamMemberNull, grpNull, locationNull]], ["_identifier", "", [""]]];
private _entity = missionNamespace getVariable _identifier;

if !(isNil "_entity") exitWith {
    _entity;
};

if (_type isNotEqualTo "") then {
    _type = typeName _type;
};

switch _type do {
    case "OBJECT": {
        private _allEntities = entities "";
        private _index = _allEntities findIf {_identifier isEqualTo (vehicleVarName _x);};
        
        if (_index isNotEqualTo -1) then {
            _allEntities select _index;
        }
        else {
            objNull;
        };
    };

    case "TEAM_MEMBER": {
        private _allAgents = agents;
        private _index = _allAgents findIf {_identifier isEqualTo (vehicleVarName (agent _x));};
        
        if (_index isNotEqualTo -1) then {
            _allAgents select _index;
        }
        else {
            teamMemberNull;
        };
    };

    case "GROUP": {
        private _allGroups = allGroups;
        private _index = _allGroups findIf {
            if (_identifier isEqualTo ([side _x, groupId _x] joinString "_")) then {
                true;
            }
            else {
                (_identifier isEqualTo (groupId _x));
            };
        };

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