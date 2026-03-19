if canSuspend exitWith {
	isNil {
		setCallArguments _this;
		setReturnValue (getCallArguments call KH_fnc_execute);
	};

	getReturnValue;
};

params [
	"_arguments", 
	["_function", {}, ["", {}]], 
	["_target", true, [true, 0, "", [], {}, objNull, teamMemberNull, grpNull, sideUnknown, locationNull]], 
	["_environment", true, [true, 0, "", [], {}]],
	["_special", false, [true, [], createHashMap]]
];

private _basic = (_target isEqualTo true) && (_special isEqualTo false);
private _subfunction = [KH_fnc_processExecution, KH_fnc_callSerializedFunction] select _basic;

if (_special isEqualTo true) then {
	_special = ["JIP", true, false, ""];
};

if (_environment isEqualType true) exitWith {
	if _basic then {
		[_arguments, [_function, false] call KH_fnc_serializeFunction, clientOwner, _environment] call _subfunction;
	}
	else {
		[_arguments, [_function, false] call KH_fnc_serializeFunction, _target, _special, "", _environment] call _subfunction;
	};
};

if !(_environment isEqualType []) then {
	_environment = [_environment];
};

private _environmentType = _environment param [0, "", [0, "", {}]];
private _environmentId = generateUid;
missionNamespace setVariable [_environmentId, true];

switch (typeName _environmentType) do {
	case "SCALAR": {
		(_environment select [1]) params [["_immediate", true, [true]], ["_timeoutRules", false, [true, 0, "", []]], ["_timeoutFunction", {}, [{}]], ["_verboseDelta", false, [true]], ["_unscheduled", true, [true]]];
		private _handlerTickCounterId = generateUid;
		private _iterationCount = false;
		
		switch (typeName _timeoutRules) do {
			case "BOOL": {
				if _timeoutRules then {
					_timeoutRules = [[1], false, false];
				}
				else {
					_timeoutRules = [0, false, false];
				};
			};

			case "SCALAR";
			case "STRING": {
				_timeoutRules = [_timeoutRules, false, false];
			};
		};

		_timeoutRules params [["_timeout", 0, [true, 0, "", []]], ["_timeoutPriority", false, [true]], ["_timeoutOnDeletion", false, [true]]];

		switch (typeName _timeout) do {
			case "BOOL": {
				if _timeout then {
					_timeout = 1;
					_iterationCount = true;
					_handlerTickCounterId = generateUid;
					missionNamespace setVariable [_handlerTickCounterId, 1];
				}
				else {
					_timeout = 0;
				};
			};

			case "STRING": {
				_timeout = ((parseNumber _timeout) - CBA_missionTime) max 0;
			};

			case "ARRAY": {
				_timeout = (_timeout select 0) max 1;
				_iterationCount = true;
				_handlerTickCounterId = generateUid;
				missionNamespace setVariable [_handlerTickCounterId, 1];
			};
		};

		([_special, _target] call KH_fnc_parseSpecialExecution) params ["_return", "_specialIdOverride"];
		private "_previousReturn";
		private _continue = true;

		if (isNil "_arguments") then {
			_arguments = [];
		};

		private _fedArguments = if _basic then {
			[_arguments, [_function, false] call KH_fnc_serializeFunction, clientOwner, _unscheduled];
		}
		else {
			[_arguments, [_function, false] call KH_fnc_serializeFunction, _target, _special, _specialIdOverride, _unscheduled];
		};

		KH_var_temporalExecutionStackMonitor set [
			_environmentId, 
			[
				[_arguments, _timeoutFunction, _environmentType, _environmentId, _return],
				{
					params ["_arguments", "_timeoutFunction", "_environmentType", "_environmentId", "_return"];
					private _handlerId = [[["TEMPORAL"], _environmentType, _environmentId, clientOwner], _return];
					_arguments call _timeoutFunction;
				},
				_handlerTickCounterId, 
				_timeout, 
				_timeoutOnDeletion
			]
		];

		if _immediate then {
			private _handlerId = [[["TEMPORAL"], _environmentType, _environmentId, clientOwner], _return];
			private _totalDelta = 0;
			private _executionTime = CBA_missionTime;
			private _executionCount = 0;

			if _iterationCount then {
				_previousReturn = _fedArguments call _subfunction;
				["KH_eve_temporalExecutionStackHandler", [_environmentId, false, false, false], true, false] call KH_fnc_triggerCbaEvent;

				if (_timeout isEqualTo 1) then {
					_continue = false;
				};
			}
			else {
				_previousReturn = _fedArguments call _subfunction;
			};
		};

		if !_continue exitWith {
			[[["TEMPORAL"], _environmentType, _environmentId, clientOwner], _return];
		};

		KH_var_temporalExecutionStackAdditions pushBack [
			[_fedArguments, _subfunction, _environmentId],
			if _iterationCount then {
				{
					params ["_fedArguments", "_subfunction"];														
					_fedArguments call _subfunction;
					["KH_eve_temporalExecutionStackHandler", [_environmentId, false, false, false], true, false] call KH_fnc_triggerCbaEvent;
				};
			}
			else {
				{
					params ["_fedArguments", "_subfunction", "_environmentId"];

					if !(missionNamespace getVariable _environmentId) exitWith {
						KH_var_temporalExecutionStackDeletions pushBackUnique _environmentId;
					};
														
					_fedArguments call _subfunction;
				};
			},
			_environmentType,
			if (_environmentType isEqualTo 0) then {
				diag_frameNo + 1;
			}
			else {
				if (_environmentType > 0) then {
					diag_tickTime + _environmentType;
				}
				else {
					diag_frameNo + (abs _environmentType);
				};
			},
			[-1, getEpoch] select _verboseDelta,
			[[["TEMPORAL"], _environmentType, _environmentId, clientOwner], _return],
			_environmentId,
			_previousReturn,
			CBA_missionTime,
			[0, 1] select _immediate
		];

		if (!_iterationCount && (_timeout isNotEqualTo 0)) then {
			private _timeoutId = generateUid;

			KH_var_temporalExecutionStackAdditions insert [
				[-1, 0] select _timeoutPriority,
				[
					[
						[_environmentId],
						{
							params ["_environmentId"];
							["KH_eve_temporalExecutionStackHandler", [_environmentId, true, true, false], true, false] call KH_fnc_triggerCbaEvent;
							KH_var_temporalExecutionStackDeletions pushBackUnique _handlerId;
						},
						_timeout,
						if (_timeout isEqualTo 0) then {
							diag_frameNo + 1;
						}
						else {
							if (_timeout > 0) then {
								diag_tickTime + _timeout;
							}
							else {
								diag_frameNo + (abs _timeout);
							};
						},
						-1,
						_timeoutId,
						_timeoutId,
						nil,
						CBA_missionTime,
						0
					]
				]
			];
		};

		[[["TEMPORAL"], _environmentType, _environmentId, clientOwner], _return];
	};

	case "CODE": {
		(_environment select [1]) params [
			["_immediate", true, [true]], 
			["_interval", 0, [0]], 
			["_timeoutRules", [[1, false], false, false, false], [true, 0, "", []]], 
			["_timeoutFunction", {}, [{}]], 
			["_verboseDelta", false, [true]], 
			["_unscheduled", true, [true]]
		];

		private _handlerTickCounterId = generateUid;
		private _iterationCount = false;
		private "_countConditionFailure";
		
		switch (typeName _timeoutRules) do {
			case "BOOL": {
				if _timeoutRules then {
					_timeoutRules = [[1], false, false, false];
				}
				else {
					_timeoutRules = [0, false, false, false];
				};
			};

			case "SCALAR";
			case "STRING": {
				_timeoutRules = [_timeoutRules, false, false, false];
			};
		};

		_timeoutRules params [["_timeout", 0, [true, 0, "", []]], ["_timeoutPriority", false, [true]], ["_timeoutOnConditionFailure", false, [true]], ["_timeoutOnDeletion", false, [true]]];

		switch (typeName _timeout) do {
			case "BOOL": {
				if _timeout then {
					_timeout = 1;
					_iterationCount = true;
					_countConditionFailure = false;
					_handlerTickCounterId = generateUid;
					missionNamespace setVariable [_handlerTickCounterId, 1];
				}
				else {
					_timeout = 0;
				};
			};

			case "STRING": {
				_timeout = ((parseNumber _timeout) - CBA_missionTime) max 0;
			};

			case "ARRAY": {
				_countConditionFailure = _timeout param [1, false, [true]];
				_timeout = (_timeout select 0) max 1;
				_iterationCount = true;
				_handlerTickCounterId = generateUid;
				missionNamespace setVariable [_handlerTickCounterId, 1];
			};
		};

		([_special, _target] call KH_fnc_parseSpecialExecution) params ["_return", "_specialIdOverride"];
		_environmentType = missionNamespace getVariable ([_environmentType, false] call KH_fnc_serializeFunction);
		private "_previousReturn";
		private _continue = true;

		if (isNil "_arguments") then {
			_arguments = [];
		};

		private _fedArguments = if _basic then {
			[_arguments, [_function, false] call KH_fnc_serializeFunction, clientOwner, _unscheduled];
		}
		else {
			[_arguments, [_function, false] call KH_fnc_serializeFunction, _target, _special, _specialIdOverride, _unscheduled];
		};

		KH_var_temporalExecutionStackMonitor set [
			_environmentId, 
			[
				[_arguments, _timeoutFunction, _environmentId, _interval, _return],
				{
					params ["_arguments", "_timeoutFunction", "_environmentId", "_interval", "_return"];
					private _handlerId = [[["TEMPORAL"], _interval, _environmentId, clientOwner], _return];
					_arguments call _timeoutFunction;
				},
				_handlerTickCounterId, 
				_timeout, 
				_timeoutOnDeletion
			]
		];

		if _immediate then {
			private _handlerId = [[["TEMPORAL"], _interval, _environmentId, clientOwner], _return];
			private _totalDelta = 0;
			private _executionTime = CBA_missionTime;
			private _executionCount = 0;

			if _iterationCount then {
				if _countConditionFailure then {
					if _timeoutOnConditionFailure then {
						if (_arguments call _environmentType) then {
							_previousReturn = _fedArguments call _subfunction;
							["KH_eve_temporalExecutionStackHandler", [_environmentId, false, false, false], true, false] call KH_fnc_triggerCbaEvent;
						}
						else {
							["KH_eve_temporalExecutionStackHandler", [_environmentId, true, true, true], true, false] call KH_fnc_triggerCbaEvent;
						};
					}
					else {
						if (_arguments call _environmentType) then {
							_previousReturn = _fedArguments call _subfunction;
							["KH_eve_temporalExecutionStackHandler", [_environmentId, false, false, false], true, false] call KH_fnc_triggerCbaEvent;
						}
						else {
							["KH_eve_temporalExecutionStackHandler", [_environmentId, false, false, true], true, false] call KH_fnc_triggerCbaEvent;
						};
					};

					if (_timeout isEqualTo 1) then {
						_continue = false;
					};
				}
				else {
					if _timeoutOnConditionFailure then {
						if (_arguments call _environmentType) then {
							_previousReturn = _fedArguments call _subfunction;
							["KH_eve_temporalExecutionStackHandler", [_environmentId, false, false, false], true, false] call KH_fnc_triggerCbaEvent;

							if (_timeout isEqualTo 1) then {
								_continue = false;
							};
						}
						else {
							["KH_eve_temporalExecutionStackHandler", [_environmentId, true, true, true], true, false] call KH_fnc_triggerCbaEvent;
						};
					}
					else {
						if (_arguments call _environmentType) then {
							_previousReturn = _fedArguments call _subfunction;
							["KH_eve_temporalExecutionStackHandler", [_environmentId, false, false, false], true, false] call KH_fnc_triggerCbaEvent;

							if (_timeout isEqualTo 1) then {
								_continue = false;
							};
						};
					};
				};
			}
			else {
				if _timeoutOnConditionFailure then {
					if (_arguments call _environmentType) then {
						_previousReturn = _fedArguments call _subfunction;
					}
					else {
						["KH_eve_temporalExecutionStackHandler", [_environmentId, true, true, true], true, false] call KH_fnc_triggerCbaEvent;
					};
				}
				else {
					if (_arguments call _environmentType) then {
						_previousReturn = _fedArguments call _subfunction;
					};
				};
			};
		};

		if !_continue exitWith {
			[[["TEMPORAL"], _interval, _environmentId, clientOwner], _return];
		};

		KH_var_temporalExecutionStackAdditions pushBack [
			[_arguments, _fedArguments, _subfunction, _environmentId, _environmentType],
			if _iterationCount then {
				if _countConditionFailure then {
					if _timeoutOnConditionFailure then {
						{
							params ["_arguments", "_fedArguments", "_subfunction", "_environmentId", "_environmentType"];

							if !(missionNamespace getVariable _environmentId) exitWith {
								KH_var_temporalExecutionStackDeletions pushBackUnique _environmentId;
							};

							if (_arguments call _environmentType) then {
								_fedArguments call _subfunction;
								["KH_eve_temporalExecutionStackHandler", [_environmentId, false, false, false], true, false] call KH_fnc_triggerCbaEvent;
							}
							else {
								["KH_eve_temporalExecutionStackHandler", [_environmentId, true, true, true], true, false] call KH_fnc_triggerCbaEvent;
							};
						};
					}
					else {
						{
							params ["_arguments", "_fedArguments", "_subfunction", "_environmentId", "_environmentType"];

							if !(missionNamespace getVariable _environmentId) exitWith {
								KH_var_temporalExecutionStackDeletions pushBackUnique _environmentId;
							};

							if (_arguments call _environmentType) then {
								_fedArguments call _subfunction;
								["KH_eve_temporalExecutionStackHandler", [_environmentId, false, false, false], true, false] call KH_fnc_triggerCbaEvent;
							}
							else {
								["KH_eve_temporalExecutionStackHandler", [_environmentId, false, false, true], true, false] call KH_fnc_triggerCbaEvent;
							};
						};
					};
				}
				else {
					if _timeoutOnConditionFailure then {
						{
							params ["_arguments", "_fedArguments", "_subfunction", "_environmentId", "_environmentType"];

							if !(missionNamespace getVariable _environmentId) exitWith {
								KH_var_temporalExecutionStackDeletions pushBackUnique _environmentId;
							};

							if (_arguments call _environmentType) then {
								_fedArguments call _subfunction;
								["KH_eve_temporalExecutionStackHandler", [_environmentId, false, false, false], true, false] call KH_fnc_triggerCbaEvent;
							}
							else {
								["KH_eve_temporalExecutionStackHandler", [_environmentId, true, true, true], true, false] call KH_fnc_triggerCbaEvent;
							};
						};
					}
					else {
						{
							params ["_arguments", "_fedArguments", "_subfunction", "_environmentId", "_environmentType"];

							if !(missionNamespace getVariable _environmentId) exitWith {
								KH_var_temporalExecutionStackDeletions pushBackUnique _environmentId;
							};

							if (_arguments call _environmentType) then {
								_fedArguments call _subfunction;
								["KH_eve_temporalExecutionStackHandler", [_environmentId, false, false, false], true, false] call KH_fnc_triggerCbaEvent;
							};
						};
					};
				};
			}
			else {
				if _timeoutOnConditionFailure then {
					{
						params ["_arguments", "_fedArguments", "_subfunction", "_environmentId", "_environmentType"];

						if !(missionNamespace getVariable _environmentId) exitWith {
							KH_var_temporalExecutionStackDeletions pushBackUnique _environmentId;
						};
						
						if (_arguments call _environmentType) then {
							_fedArguments call _subfunction;
						}
						else {
							["KH_eve_temporalExecutionStackHandler", [_environmentId, true, true, true], true, false] call KH_fnc_triggerCbaEvent;
						};
					};
				}
				else {
					{
						params ["_arguments", "_fedArguments", "_subfunction", "_environmentId", "_environmentType"];

						if !(missionNamespace getVariable _environmentId) exitWith {
							KH_var_temporalExecutionStackDeletions pushBackUnique _environmentId;
						};

						if (_arguments call _environmentType) then {
							_fedArguments call _subfunction;
						}
					};
				};
			},
			_interval,
			if (_interval isEqualTo 0) then {
				diag_frameNo + 1;
			}
			else {
				if (_interval > 0) then {
					diag_tickTime + _interval;
				}
				else {
					diag_frameNo + (abs _interval);
				};
			},
			[-1, getEpoch] select _verboseDelta,
			[[["TEMPORAL"], _interval, _environmentId, clientOwner], _return],
			_environmentId,
			_previousReturn,
			CBA_missionTime,
			[0, 1] select _immediate
		];

		if (!_iterationCount && (_timeout isNotEqualTo 0)) then {
			private _timeoutId = generateUid;

			KH_var_temporalExecutionStackAdditions insert [
				[-1, 0] select _timeoutPriority,
				[
					[
						[_environmentId],
						{
							params ["_environmentId"];
							["KH_eve_temporalExecutionStackHandler", [_environmentId, true, true, false], true, false] call KH_fnc_triggerCbaEvent;
							KH_var_temporalExecutionStackDeletions pushBackUnique _handlerId;
						},
						_timeout,
						if (_timeout isEqualTo 0) then {
							diag_frameNo + 1;
						}
						else {
							if (_timeout > 0) then {
								diag_tickTime + _timeout;
							}
							else {
								diag_frameNo + (abs _timeout);
							};
						},
						-1,
						_timeoutId,
						_timeoutId,
						nil,
						CBA_missionTime,
						0
					]
				]
			];
		};

		[[["TEMPORAL"], _interval, _environmentId, clientOwner], _return];
	};

	case "STRING": {
		private _unscheduled = _environment param [1, true, [true]];
		([_special, _target] call KH_fnc_parseSpecialExecution) params ["_return", "_specialIdOverride"];
		_environmentType = parseNumber _environmentType;

		private _fedArguments = if _basic then {
			[_arguments, [_function, false] call KH_fnc_serializeFunction, clientOwner, _unscheduled];
		}
		else {
			[_arguments, [_function, false] call KH_fnc_serializeFunction, _target, _special, _specialIdOverride, _unscheduled];
		};

		if (_environmentType isEqualTo 0) exitWith {
			_fedArguments call _subFunction;
			[[missionNamespace, _environmentId, clientOwner], _return];
		};

		KH_var_temporalExecutionStackAdditions pushBack [
			[_fedArguments, _subfunction, _environmentId],
			{
				params ["_fedArguments", "_subfunction", "_environmentId"];

				if !(missionNamespace getVariable _environmentId) exitWith {																											
					KH_var_temporalExecutionStackDeletions pushBackUnique _environmentId;
				};
												
				_fedArguments call _subfunction;
				KH_var_temporalExecutionStackDeletions pushBackUnique _environmentId;
			},
			_environmentType,
			if (_environmentType > 0) then {
				diag_tickTime + _environmentType;
			}
			else {
				diag_frameNo + (abs _environmentType);
			},
			-1,
			[[missionNamespace, _environmentId, clientOwner], _return],
			_environmentId,
			nil,
			CBA_missionTime,
			0
		];

		[[missionNamespace, _environmentId, clientOwner], _return];
	};

	default {
		nil;
	};
};