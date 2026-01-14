params [["_identifier", "", [""]]];
private _file = ["mission_recording_", _identifier] joinString "";
KH_var_recordedUnitData = _file readKhData ["unitData", createHashMap];
KH_var_recordedGroupData = _file readKhData ["groupData", createHashMap];
KH_var_recordedObjectData = _file readKhData ["objectData", createHashMap];
KH_var_recordedScenarioData = _file readKhData ["scenarioData", []];
KH_var_recordedUnits = createHashMap;
KH_var_recordedGroups = createHashMap;
KH_var_recordedObjects = createHashMap;

{
    private _attributes = (((_y select 0) select 1) get "Header") select 1;
    private _object = createVehicle [_attributes select 3, [0, 0, 0], [], 0, "CAN_COLLIDE"];
    [_object, _attributes, [], true] call KH_fnc_setObjectAttributes;
    KH_var_recordedObjects set [_x, _object];
} forEach KH_var_recordedObjectData;

{
    private _attributes = (((_y select 0) select 1) get "Header") select 1;
    private _group = createGroup [_attributes select 3, false];
    [_group, _attributes, []] call KH_fnc_setGroupAttributes;
    KH_var_recordedGroups set [_x, _group];
} forEach KH_var_recordedGroupData;

{
    private _attributes = (((_y select 0) select 1) get "Header") select 3;
    private _unit = (KH_var_recordedGroups get (((((_y select 0) select 1) get "Group") select 0) select 1)) createUnit [_attributes select 3, [0, 0, 0], [], 0, "CAN_COLLIDE"];
    [_unit, _attributes, [], true] call KH_fnc_setUnitAttributes;
    _unit enableAIFeature ["AUTOTARGET", false];
    _unit enableAIFeature ["MOVE", false];
    _unit enableAIFeature ["TARGET", false];
    _unit enableAIFeature ["TEAMSWITCH", false];
    _unit enableAIFeature ["WEAPONAIM", false];
    _unit enableAIFeature ["ANIM", true];
    _unit enableAIFeature ["FSM", false];
    _unit enableAIFeature ["AIMINGERROR", false];
    _unit enableAIFeature ["SUPPRESSION", false];
    _unit enableAIFeature ["CHECKVISIBLE", false];
    _unit enableAIFeature ["AUTOCOMBAT", false];
    _unit enableAIFeature ["COVER", false];
    _unit enableAIFeature ["PATH", false];
    _unit enableAIFeature ["MINEDETECTION", false];
    _unit enableAIFeature ["LIGHTS", true];
    _unit enableAIFeature ["NVG", true];
    _unit enableAIFeature ["RADIOPROTOCOL", false];
    _unit enableAIFeature ["FIREWEAPON", false];
    _unit setBehaviourStrong "CARELESS";
    _unit setCombatMode "BLUE";
    _unit allowDamage false;
    KH_var_recordedUnits set [_x, _unit];
} forEach KH_var_recordedUnitData;

if (KH_var_recordedScenarioData isNotEqualTo []) then {
    [(((KH_var_recordedScenarioData select 0) select 1) get "Header") select 2, []] call KH_fnc_setMissionAttributes;
};

[
    [],
    {
        {
            private _unit = KH_var_recordedUnits get _x;

            if (_y isEqualTo []) then {
                continue;
            };

            private _currentRecordedData = _y select (_unit getVariable ["KH_var_currentRecordingIndex", 0]);

            if (CBA_missionTime >= ((_y param [(_unit getVariable ["KH_var_currentRecordingIndex", 0]) + 1, [999999]]) select 0)) then {
                private _nextData = _y param [(_unit getVariable ["KH_var_currentRecordingIndex", 0]) + 1, []];

                if (_nextData isNotEqualTo []) then {
                    [_unit] joinSilent (KH_var_recordedGroups get ((((_nextData select 1) get "Group") select 0) select 1));
                    [_unit, ((_nextData select 1) get "Header") select 3, [], true] call KH_fnc_setUnitAttributes;
                    _unit setVariable ["KH_var_currentRecordingIndex", (_unit getVariable ["KH_var_currentRecordingIndex", 0]) + 1];
                };

                continue;
            };

            private _currentData = _currentRecordedData select 1;

            {
                _x params ["_time", "_data"];

                if (_time <= CBA_missionTime) then {
                    _unit switchMove [_data select 0];
                }
                else {
                    break;
                };
            } forEach (_currentData get "Animation");

            {
                _x params ["_time", "_data"];

                if (_time <= CBA_missionTime) then {
                    _unit setCaptive _data;
                }
                else {
                    break;
                };
            } forEach (_currentData get "Captive");

            {
                _x params ["_time", "_data"];

                if (_time <= CBA_missionTime) then {
                    _unit setDamage (_data select 0);

                    {
                        _unit setHitPointDamage [_x select 0, _x select 1];
                    } forEach (_data select 1);
                }
                else {
                    break;
                };
            } forEach (_currentData get "Damage");

            {
                _x params ["_time", "_data"];

                if (_time <= CBA_missionTime) then {
                    _unit setVectorDirAndUp _data;
                }
                else {
                    break;
                };
            } forEach (_currentData get "Direction");

            {
                _x params ["_time", "_data"];

                if (_time <= CBA_missionTime) then {
                    _unit forceWeaponFire [_data select 1, _data select 2];
                }
                else {
                    break;
                };
            } forEach (_currentData get "Fire");

            {
                _x params ["_time", "_data"];

                if (_time <= CBA_missionTime) then {
                    [_unit, [KH_var_recordedObjects get (_data select 1), _data select 0, _data select 3], true] call KH_fnc_setUnitVehicleSlot;
                }
                else {
                    break;
                };
            } forEach (_currentData get "GetIn");

            {
                _x params ["_time"];

                if (_time <= CBA_missionTime) then {
                    moveOut _unit;
                }
                else {
                    break;
                };
            } forEach (_currentData get "GetOut");

            {
                _x params ["_time", "_data"];

                if (_time <= CBA_missionTime) then {
                    _unit switchGesture _data;
                }
                else {
                    break;
                };
            } forEach (_currentData get "Gesture");

            {
                _x params ["_time", "_data"];

                if (_time <= CBA_missionTime) then {
                    [_unit] joinSilent (KH_var_recordedGroups get _data);
                }
                else {
                    break;
                };
            } forEach (_currentData get "Group");

            {
                _x params ["_time", "_data"];

                if (_time <= CBA_missionTime) then {
                    _unit setUnconscious _data;
                }
                else {
                    break;
                };
            } forEach (_currentData get "Incapacitation");

            {
                _x params ["_time", "_data"];

                if (_time <= CBA_missionTime) then {
                    _unit setOxygenRemaining _data;
                }
                else {
                    break;
                };
            } forEach (_currentData get "Oxygen");

            {
                _x params ["_time", "_data"];

                if (isNull (objectParent _unit)) then {
                    if (_time <= CBA_missionTime) then {
                        _unit setPosATL [(_data select 0) select 0, (_data select 0) select 1, _data select 1];
                    }
                    else {
                        break;
                    };
                };
            } forEach (_currentData get "Position");

            {
                _x params ["_time", "_data"];

                if (_time <= CBA_missionTime) then {
                    _unit reload [_data select 1, (_data select 2) select 0];
                }
                else {
                    break;
                };
            } forEach (_currentData get "Reload");

            {
                _x params ["_time", "_data"];

                if (_time <= CBA_missionTime) then {
                    [_unit, [KH_var_recordedObjects get (_data select 1), (_data select 2) select 0, (_data select 2) param [1, []]], true] call KH_fnc_setUnitVehicleSlot;
                }
                else {
                    break;
                };
            } forEach (_currentData get "SeatSwitch");

            {
                _x params ["_time", "_data"];

                if (_time <= CBA_missionTime) then {
                    _unit setUnitPos _data;
                }
                else {
                    break;
                };
            } forEach (_currentData get "Stance");

            {
                _x params ["_time", "_data"];

                if (isNull (objectParent _unit)) then {
                    if (_time <= CBA_missionTime) then {
                        _unit assignTeam _data;
                    }
                    else {
                        break;
                    };
                };
            } forEach (_currentData get "Team");

            {
                _x params ["_time", "_data"];

                if (_time <= CBA_missionTime) then {
                    _unit actionNow [["TurnIn", "TurnOut"] select _data, objectParent _unit];
                }
                else {
                    break;
                };
            } forEach (_currentData get "VehicleTurnState");

            {
                _x params ["_time", "_data"];

                if (isNull (objectParent _unit)) then {
                    if (_time <= CBA_missionTime) then {
                        _unit setVelocity _data;
                    }
                    else {
                        break;
                    };
                };
            } forEach (_currentData get "Velocity");

            {
                _x params ["_time", "_data"];

                if (_time <= CBA_missionTime) then {
                    _unit selectWeapon [_data select 0, _data select 2, _data select 1];
                }
                else {
                    break;
                };
            } forEach (_currentData get "Weapon");

            {
                private _currentRemovalData = +(_currentData get _x);

                [
                    _currentRemovalData,
                    {
                        ((_x select 0) <= CBA_missionTime);
                    }
                ] call KH_fnc_deleteArrayElements;

                _currentData set [_x, _currentRemovalData];
            } forEach [
                "Animation", 
                "Captive", 
                "Damage", 
                "Direction", 
                "Fire", 
                "GetIn", 
                "GetOut", 
                "Gesture", 
                "Group", 
                "Incapacitation", 
                "Oxygen", 
                "Position", 
                "Reload", 
                "SeatSwitch", 
                "Stance",
                "Team",
                "VehicleTurnState", 
                "Velocity", 
                "Weapon"
            ];
        } forEach KH_var_recordedUnitData;
    },
    true,
    0,
    false
] call KH_fnc_execute;