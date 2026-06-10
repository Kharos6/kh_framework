params [["_type", objNull, ["", objNull, teamMemberNull, grpNull, locationNull]], ["_identifier", "", [""]]];
private _entity = missionNamespace getVariable _identifier;

if !(isNil "_entity") exitWith {
    _entity;
};

if (_type isNotEqualTo "") then {
    _type = typeName _type;
};

switch _type do {
    case "OBJECT";
    case "TEAM_MEMBER": {
        missionNamespace getVariable [_identifier, objNull];
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