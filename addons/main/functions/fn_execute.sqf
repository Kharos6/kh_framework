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

_function = [_function, false] call KH_fnc_serializeFunction;
private _basic = (_target isEqualTo true) && (_special isEqualTo false);

private _subfunction = if _basic then {
	KH_fnc_callSerializedFunction;
}
else {
	{
		params ["_arguments", "_function", "_target", "_special", "_specialIdOverride", "_unscheduled"];

		if (_special isEqualTo false) exitWith {
			["KH_eve_execution", [_arguments, _function, clientOwner, _unscheduled], _target, false] call KH_fnc_triggerCbaEvent;
		};

		if (_special isEqualType createHashMap) exitWith {
			private _argumentsId = generateUid;
			missionNamespace setVariable [_argumentsId, _arguments];

			[
				"KH_eve_execution", 
				[
					_arguments, 
					[compile ([_special, " call ['", _function, "', missionNamespace getVariable '", _argumentsId, "'];"] joinString ""), false] call KH_fnc_serializeFunction, 
					clientOwner, 
					true
				], 
				_target, 
				false
			] call KH_fnc_triggerCbaEvent;
		};

		private _specialType = _special param [0, "", [""]];

		switch _specialType do {
			case "JIP": {
				private _dependency = _special param [1, true, [true, 0, "", [], {}, objNull, teamMemberNull, grpNull]];
				private _unitRequired = _special param [2, false, [true]];
				private _jipId = _special param [3, "", [""]];

				_jipId = if (_jipId isNotEqualTo "") then {
					_jipId;
				}
				else {
					if (_specialIdOverride isNotEqualTo "") then {
						_specialIdOverride;
					}
					else {
						generateUid;
					};
				};

				missionNamespace setVariable [_jipId, true, 2];

				[
					"KH_eve_execution", 
					[_arguments, _function, clientOwner, _unscheduled],
					_target, 
					[_dependency, _unitRequired, _jipId]
				] call KH_fnc_triggerCbaEvent;
			};

			case "CALLBACK": {
				private _callbackArguments = _special param [1, []];
				private _callbackFunction = _special param [2, {}, ["", {}]];
				private _callbackId = generateUid;

				[
					"CBA",
					_callbackId,
					[_arguments, _function, _unscheduled],
					{
						_args params ["_arguments", "_function", "_unscheduled"];
						private _argsCallback = _this;

						if _unscheduled then {
							if (isNil "_arguments") then {
								call (missionNamespace getVariable _function);
							}
							else {
								_arguments call (missionNamespace getVariable _function);
							};
						}
						else {
							if (isNil "_arguments") then {
								[] spawn (missionNamespace getVariable _function);
							}
							else {
								_arguments spawn (missionNamespace getVariable _function);
							};
						};

						[_handlerId] call KH_fnc_removeHandler;
					}
				] call KH_fnc_addEventHandler;

				[
					"KH_eve_registerCallback", 
					[_callbackArguments, [_callbackFunction, false] call KH_fnc_serializeFunction, clientOwner, _unscheduled, _callbackId], 
					_target, 
					false
				] call KH_fnc_triggerCbaEvent;
			};

			case "PERSISTENT": {
				private _object = _special param [1, objNull, [objNull]];
				private _sendoffArguments = _special param [2];
				private _sendoffFunction = _special param [3, {}, ["", {}]];
				private _persistentExecutionId = _special param [4, "", [""]];

				_persistentExecutionId = if (_persistentExecutionId isNotEqualTo "") then {
					_persistentExecutionId;
				}
				else {
					if (_specialIdOverride isNotEqualTo "") then {
						_specialIdOverride;
					}
					else {
						generateUid;
					};
				};

				_object setVariable [_persistentExecutionId, true, true];
				["KH_eve_execution", [_arguments, _function, clientOwner, _unscheduled], _target, false] call KH_fnc_triggerCbaEvent;

				[
					"KH_eve_persistentExecutionSetup", 
					[_arguments, _function, _object, _sendoffArguments, [_sendoffFunction, false] call KH_fnc_serializeFunction, clientOwner, _unscheduled, _persistentExecutionId]
				] call CBA_fnc_serverEvent;

				[_object, _persistentExecutionId, true];
			};

			case "PLAYER_PRESENCE": {
				private _object = _special param [1, objNull, [objNull]];
				private _present = _special param [2, true, [true]];
				private _distance = _special param [3, 0, [0]];
				private _jip = _special param [4, true, [true]];
				private _nearId = _special param [5, "", [""]];
				
				_nearId = if (_nearId isNotEqualTo "") then {
					_nearId;
				}
				else {
					if (_specialIdOverride isNotEqualTo "") then {
						_specialIdOverride;
					}
					else {
						generateUid;
					};
				};

				missionNamespace setVariable [_nearId, true, 2];
				["KH_eve_execution", [_arguments, _function, clientOwner, _unscheduled], _target, false] call KH_fnc_triggerCbaEvent;
				["KH_eve_playerPresenceExecutionSetup", [_arguments, _function, clientOwner, _unscheduled, _object, _present, _distance, _nearId, +KH_var_allPlayerControlledUnits, _jip]] call CBA_fnc_serverEvent;
				[missionNamespace, _nearId, 2];
			};

			default {
				nil;
			};
		};
	};
};

if (_special isEqualTo true) then {
	_special = ["JIP", true, false, ""];
};

if (_environment isEqualType true) exitWith {
	if _basic then {
		[_arguments, _function, clientOwner, _environment] call _subfunction;
	}
	else {
		[_arguments, _function, _target, _special, "", _environment] call _subfunction;
	};
};

if !(_environment isEqualType []) then {
	_environment = [_environment];
};

private _environmentType = _environment param [0, "", [0, "", {}]];
private _environmentId = generateUid;
missionNamespace setVariable [_environmentId, true];

private _specialParser = {
	params ["_special", "_target"];

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
};

switch (typeName _environmentType) do {
	case "SCALAR": {
		private _immediate = _environment param [1, true, [true]];
		private _timeoutRules = _environment param [2, false, [true, 0, "", []]];
		private _timeoutFunction = _environment param [3, {}, [{}]];
		private _verboseDelta = _environment param [4, false, [true]];
		private _unscheduled = _environment param [5, true, [true]];
		private _handlerTickCounterId = generateUid;
		private _iterationCount = false;
		
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

		([_special, _target] call _specialParser) params ["_return", "_specialIdOverride"];
		private "_previousReturn";
		private _continue = true;

		if (isNil "_arguments") then {
			_arguments = [];
		};

		private _fedArguments = if _basic then {
			[_arguments, _function, clientOwner, _unscheduled];
		}
		else {
			[_arguments, _function, _target, _special, _specialIdOverride, _unscheduled];
		};

		KH_var_temporalExecutionStackMonitor set [
			_environmentId, 
			[
				[_arguments, _timeoutFunction, _environmentId, _return],
				{
					params ["_arguments", "_timeoutFunction", "_environmentId", "_return"];
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
				["KH_eve_temporalExecutionStackHandler", [_environmentId, false, false, false]] call CBA_fnc_localEvent;

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
					["KH_eve_temporalExecutionStackHandler", [_environmentId, false, false, false]] call CBA_fnc_localEvent;
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
							["KH_eve_temporalExecutionStackHandler", [_environmentId, true, true, false]] call CBA_fnc_localEvent;
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
		private _immediate = _environment param [1, true, [true]];
		private _interval = _environment param [2, 0, [0]];
		private _timeoutRules = _environment param [3, [[1, false], false, false, false], [true, 0, "", []]];
		private _timeoutFunction = _environment param [4, {}, [{}]];
		private _verboseDelta = _environment param [5, false, [true]];
		private _unscheduled = _environment param [6, true, [true]];
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

		([_special, _target] call _specialParser) params ["_return", "_specialIdOverride"];
		_environmentType = missionNamespace getVariable ([_environmentType, false] call KH_fnc_serializeFunction);
		private "_previousReturn";
		private _continue = true;

		if (isNil "_arguments") then {
			_arguments = [];
		};

		private _fedArguments = if _basic then {
			[_arguments, _function, clientOwner, _unscheduled];
		}
		else {
			[_arguments, _function, _target, _special, _specialIdOverride, _unscheduled];
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
							["KH_eve_temporalExecutionStackHandler", [_environmentId, false, false, false]] call CBA_fnc_localEvent;
						}
						else {
							["KH_eve_temporalExecutionStackHandler", [_environmentId, true, true, true]] call CBA_fnc_localEvent;
						};
					}
					else {
						if (_arguments call _environmentType) then {
							_previousReturn = _fedArguments call _subfunction;
							["KH_eve_temporalExecutionStackHandler", [_environmentId, false, false, false]] call CBA_fnc_localEvent;
						}
						else {
							["KH_eve_temporalExecutionStackHandler", [_environmentId, false, false, true]] call CBA_fnc_localEvent;
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
							["KH_eve_temporalExecutionStackHandler", [_environmentId, false, false, false]] call CBA_fnc_localEvent;

							if (_timeout isEqualTo 1) then {
								_continue = false;
							};
						}
						else {
							["KH_eve_temporalExecutionStackHandler", [_environmentId, true, true, true]] call CBA_fnc_localEvent;
						};
					}
					else {
						if (_arguments call _environmentType) then {
							_previousReturn = _fedArguments call _subfunction;
							["KH_eve_temporalExecutionStackHandler", [_environmentId, false, false, false]] call CBA_fnc_localEvent;

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
						["KH_eve_temporalExecutionStackHandler", [_environmentId, true, true, true]] call CBA_fnc_localEvent;
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

							if (_arguments call _environmentType) then {
								_fedArguments call _subfunction;
								["KH_eve_temporalExecutionStackHandler", [_environmentId, false, false, false]] call CBA_fnc_localEvent;
							}
							else {
								["KH_eve_temporalExecutionStackHandler", [_environmentId, true, true, true]] call CBA_fnc_localEvent;
							};
						};
					}
					else {
						{
							params ["_arguments", "_fedArguments", "_subfunction", "_environmentId", "_environmentType"];

							if (_arguments call _environmentType) then {
								_fedArguments call _subfunction;
								["KH_eve_temporalExecutionStackHandler", [_environmentId, false, false, false]] call CBA_fnc_localEvent;
							}
							else {
								["KH_eve_temporalExecutionStackHandler", [_environmentId, false, false, true]] call CBA_fnc_localEvent;
							};
						};
					};
				}
				else {
					if _timeoutOnConditionFailure then {
						{
							params ["_arguments", "_fedArguments", "_subfunction", "_environmentId", "_environmentType"];

							if (_arguments call _environmentType) then {
								_fedArguments call _subfunction;
								["KH_eve_temporalExecutionStackHandler", [_environmentId, false, false, false]] call CBA_fnc_localEvent;
							}
							else {
								["KH_eve_temporalExecutionStackHandler", [_environmentId, true, true, true]] call CBA_fnc_localEvent;
							};
						};
					}
					else {
						{
							params ["_arguments", "_fedArguments", "_subfunction", "_environmentId", "_environmentType"];

							if (_arguments call _environmentType) then {
								_fedArguments call _subfunction;
								["KH_eve_temporalExecutionStackHandler", [_environmentId, false, false, false]] call CBA_fnc_localEvent;
							};
						};
					};
				};
			}
			else {
				if _timeoutOnConditionFailure then {
					{
						params ["_arguments", "_fedArguments", "_subfunction", "_environmentId", "_environmentType"];

						if (_arguments call _environmentType) then {
							_fedArguments call _subfunction;
						}
						else {
							["KH_eve_temporalExecutionStackHandler", [_environmentId, true, true, true]] call CBA_fnc_localEvent;
						};
					};
				}
				else {
					{
						params ["_arguments", "_fedArguments", "_subfunction", "_environmentId", "_environmentType"];

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
							["KH_eve_temporalExecutionStackHandler", [_environmentId, true, true, false]] call CBA_fnc_localEvent;
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
		([_special, _target] call _specialParser) params ["_return", "_specialIdOverride"];
		_environmentType = parseNumber _environmentType;

		private _fedArguments = if _basic then {
			[_arguments, _function, clientOwner, _unscheduled];
		}
		else {
			[_arguments, _function, _target, _special, _specialIdOverride, _unscheduled];
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