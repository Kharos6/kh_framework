params [["_state", true, [true]], ["_headlessClientBlacklist", [], [[]]], ["_entityBlacklist", [], [[]]], ["_includeServer", true, [true]], ["_distributionInterval", 60, [0]], ["_recreate", false, [true]], ["_init", {}, [{}]]];
KH_var_headlessClientOffloading = _state;
KH_var_headlessClientOffloadingClientBlacklist = _headlessClientBlacklist;
KH_var_headlessClientOffloadingEntityBlacklist = _entityBlacklist;
KH_var_headlessClientOffloadingIncludeServer = _includeServer;
KH_var_headlessClientOffloadingDistributionInterval = [_distributionInterval, 1] select (_distributionInterval < 1);
KH_var_headlessClientOffloadingRecreate = _recreate;
KH_var_headlessClientOffloadingInit = _init;

if (_state && (isNil "KH_var_headlessClientOffloadingSet")) then {
    KH_var_headlessClientOffloadingSet = true;
    KH_var_headlessClientOffloadingIndex = -1;
    
    [
        [-1],
        {
            params ["_time"];
            if (!KH_var_headlessClientOffloading || (_time < CBA_missionTime)) exitWith {};
            _this set [0, CBA_missionTime + KH_var_headlessClientOffloadingDistributionInterval];
            private _headlessClients = (KH_var_allHeadlessUnits select {!((vehicleVarName _x) in KH_var_headlessClientOffloadingClientBlacklist);}) apply {owner _x;};

            if ((count _headlessClients) isNotEqualTo 0) then {
                [
                    [
                        _headlessClients,
                        allUnits select {
                            !(_x in KH_var_headlessClientOffloadingEntityBlacklist) && !((group _x) in KH_var_headlessClientOffloadingEntityBlacklist) && !(_x getVariable ["KH_var_headlessClientOffloaded", false]) && !(isPlayer _x);
                        },
                        (agents apply {agent _x;}) select {!(_x in KH_var_headlessClientOffloadingEntityBlacklist) && !(_x getVariable ["KH_var_headlessClientOffloaded", false]) && !(isPlayer _x);}
                    ],
                    {
                        params ["_headlessClients", "_units", "_agents"];
                        private _groups = [];

                        {
                            _groups pushBackUnique (group _x);
                        } forEach _units;

                        private _groupCount = count _groups;
                        private _agentCount = count _agents;
                        private _validMachines = [_headlessClients, _headlessClients + [2]] select KH_var_headlessClientOffloadingIncludeServer;
                        private _validMachineCount = count _validMachines;
                        KH_var_headlessClientOffloadingIndex = [KH_var_headlessClientOffloadingIndex + 1, -1] select (KH_var_headlessClientOffloadingIndex >= (_validMachineCount - 1));

                        if (KH_var_headlessClientOffloadingIndex > 0) then {
                            _validMachines = (_validMachines select [KH_var_headlessClientOffloadingIndex]) + (_validMachines select [0, KH_var_headlessClientOffloadingIndex]);
                        };

                        if (_groupCount isNotEqualTo 0) then {
                            private _distribution = floor (_groupCount / _validMachineCount);
                            private _index = 0;
                            private _previousIndex = 0;

                            {
                                if (_index isEqualTo _previousIndex) then {
                                    break;    
                                };

                                [
                                    _groups select [[_index, _distribution], [_index]] select (_forEachIndex isEqualTo (_validMachineCount - 1)),
                                    _x,
                                    KH_var_headlessClientOffloadingRecreate,
                                    KH_var_headlessClientOffloadingInit
                                ] call KH_Fnc_setOwnership;

                                _previousIndex = _index;
                                _index = (_index + _distribution) min (_groupCount - 1);
                            } forEach _validMachines;
                        };

                        if (_agentCount isNotEqualTo 0) then {
                            private _distribution = floor (_agentCount / _validMachineCount);
                            private _index = 0;
                            private _previousIndex = 0;

                            {
                                if (_index isEqualTo _previousIndex) then {
                                    break;    
                                };

                                [
                                    _agents select [[_index, _distribution], [_index]] select (_forEachIndex isEqualTo (_validMachineCount - 1)),
                                    _x,
                                    KH_var_headlessClientOffloadingRecreate,
                                    KH_var_headlessClientOffloadingInit
                                ] call KH_Fnc_setOwnership;
                                
                                _previousIndex = _index;
                                _index = (_index + _distribution) min (_agentCount - 1);
                            } forEach _validMachines;
                        };

                        {
                            _x setVariable ["KH_var_headlessClientOffloaded", true];
                        } forEach (_units + _agents);
                    },
                    true,
                    "-2",
                    false
                ] call KH_fnc_execute;
            };
        },
        true,
        0,
        false
    ] call KH_fnc_execute;
};