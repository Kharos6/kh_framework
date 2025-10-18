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

_function = [_function, false] call KH_fnc_parseFunction;
private _basic = (_target isEqualTo true) && (_special isEqualTo false);

private _subfunction = if _basic then {
	KH_fnc_callParsedFunction;
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
					[compile ([_special, " call ['", _function, "', missionNamespace getVariable '", _argumentsId, "'];"] joinString ""), false] call KH_fnc_parseFunction, 
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

				[
					"KH_eve_execution", 
					[_arguments, _function, clientOwner, _unscheduled],
					_target, 
					[
						_dependency, 
						_unitRequired, 
						if (_jipId isNotEqualTo "") then {
							_jipId;
						}
						else {
							if (_specialIdOverride isNotEqualTo "") then {
								_specialIdOverride;
							}
							else {
								generateUid;
							};
						}
					]
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
						private _argsCallback = param [1];

						if _unscheduled then {
							_arguments call (missionNamespace getVariable _function);
						}
						else {
							_arguments spawn (missionNamespace getVariable _function);
						};

						[_handlerId] call KH_fnc_removeHandler;
					}
				] call KH_fnc_addEventHandler;

				[
					"KH_eve_registerCallback", 
					[_callbackArguments, [_callbackFunction, false] call KH_fnc_parseFunction, clientOwner, _unscheduled, _callbackId], 
					_target, 
					false
				] call KH_fnc_triggerCbaEvent;
			};

			case "PERSISTENT": {
				private _immediate = _special param [1, true, [true]];
				private _sendoffArguments = _special param [2];
				private _sendoffFunction = _special param [3, {}, ["", {}]];
				private _persistentExecutionId = _special param [4, "", [""]];

				if (_target isEqualType teamMemberNull) then {
					_target = agent _target;
				};

				if (_persistentExecutionId isNotEqualTo "") then {
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

				if _immediate then {
					["KH_eve_execution", [_arguments, _function, clientOwner, _unscheduled], _target, false] call KH_fnc_triggerCbaEvent;
				};

				[
					"KH_eve_persistentExecutionSetup", 
					[_arguments, _function, _target, _sendoffArguments, [_sendoffFunction, false] call KH_fnc_parseFunction, clientOwner, _unscheduled, _persistentExecutionId], 
					"SERVER", 
					false
				] call KH_fnc_triggerCbaEvent;

				[_target, _persistentExecutionId, true];
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
		private _timeout = _environment param [2, 0, [0]];
		private _unscheduled = _environment param [3, true, [true]];
		([_special, _target] call _specialParser) params ["_return", "_specialIdOverride"];
		private "_previousReturn";

		private _fedArguments = if _basic then {
			[_arguments, _function, clientOwner, _unscheduled];
		}
		else {
			[_arguments, _function, _target, _special, _specialIdOverride, _unscheduled];
		};

		if _immediate then {
			private _handlerId = [missionNamespace, _environmentId, clientOwner];
			private _totalDelta = 0;
			private _executionTime = CBA_missionTime;
			private _executionCount = 0;
			_previousReturn = _fedArguments call _subfunction;
		};

		KH_var_temporalExecutionStackAdditions pushBack [
			[_fedArguments, _subfunction, _environmentId],
			{
				params ["_fedArguments", "_subfunction", "_environmentId"];

				if !(missionNamespace getVariable _environmentId) exitWith {																											
					KH_var_temporalExecutionStackDeletions pushBackUnique _environmentId;
				};
													
				_fedArguments call _subfunction;
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
			-1,
			[[missionNamespace, _environmentId, clientOwner], _return],
			_environmentId,
			_previousReturn,
			CBA_missionTime,
			[0, 1] select _immediate
		];

		if (_timeout isNotEqualTo 0) then {
			private _timeoutId = generateUid;

			KH_var_temporalExecutionStackAdditions pushBack [
				[_environmentId],
				{
					params ["_environmentId"];
					KH_var_temporalExecutionStackDeletions pushBackUnique _environmentId;
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
			];
		};

		[[missionNamespace, _environmentId, clientOwner], _return];
	};

	case "CODE": {
		private _immediate = _environment param [1, true, [true]];
		private _timeout = _environment param [2, 0, [0]];
		private _interval = _environment param [3, 0, [0]];
		private _fireOnce = _environment param [4, true, [true]];
		private _unscheduled = _environment param [5, true, [true]];
		([_special, _target] call _specialParser) params ["_return", "_specialIdOverride"];
		_environmentType = missionNamespace getVariable ([_environmentType, false] call KH_fnc_parseFunction);
		private _continue = true;

		private _fedArguments = if _basic then {
			[_arguments, _function, clientOwner, _unscheduled];
		}
		else {
			[_arguments, _function, _target, _special, _specialIdOverride, _unscheduled];
		};

		if (isNil "_arguments") then {
			_arguments = [];
		};

		if _immediate then {
			private _handlerId = [missionNamespace, _environmentId, clientOwner];
			private _totalDelta = 0;
			private _executionTime = CBA_missionTime;
			private _executionCount = 0;

			if (_arguments call _environmentType) then {
				_fedArguments call _subfunction;

				if _fireOnce then {
					_continue = false;
				};
			};
		};

		if !_continue exitWith {
			[[missionNamespace, _environmentId, clientOwner], _return];
		};

		KH_var_temporalExecutionStackAdditions pushBack [
			[_arguments, _fedArguments, _subfunction, _environmentId, _environmentType, _fireOnce],
			{
				params ["_arguments", "_fedArguments", "_subfunction", "_environmentId", "_environmentType", "_fireOnce"];

				if !(missionNamespace getVariable _environmentId) exitWith {																											
					KH_var_temporalExecutionStackDeletions pushBackUnique _environmentId;
				};

				if (_arguments call _environmentType) then {												
					_fedArguments call _subfunction;

					if _fireOnce then {
						KH_var_temporalExecutionStackDeletions pushBackUnique _environmentId;
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
			-1,
			[[missionNamespace, _environmentId, clientOwner], _return],
			_environmentId,
			_previousReturn,
			CBA_missionTime,
			[0, 1] select _immediate
		];

		if (_timeout isNotEqualTo 0) then {
			private _timeoutId = generateUid;
			
			KH_var_temporalExecutionStackAdditions pushBack [
				[_environmentId],
				{
					params ["_environmentId"];
					KH_var_temporalExecutionStackDeletions pushBackUnique _environmentId;
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
			];
		};

		[[missionNamespace, _environmentId, clientOwner], _return];
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