if canSuspend exitWith {
	private "_return";
	isNil {_return = ([_x] apply KH_fnc_execute) select 0;};
	_return;
};

params [
	"_arguments", 
	["_function", {}, ["", [], {}]], 
	["_target", true, [true, 0, "", [], {}, objNull, teamMemberNull, grpNull, sideUnknown, locationNull]], 
	["_environment", true, [true, 0, "", [], {}]],
	["_special", false, [true, [], createHashMap]]
];

private _subfunction = {
	params ["_arguments", "_function", "_target", "_special", "_specialIdOverride", "_unscheduled"];

	if (_special isEqualTo false) exitWith {
		["KH_eve_execution", [_arguments, _function, clientOwner, _unscheduled], _target, false] call KH_fnc_triggerCbaEvent;
	};

	if (_special isEqualType createHashMap) exitWith {
		private _argumentsId = call KH_fnc_generateUid;
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
							call KH_fnc_generateUid;
						};
					}
				]
			] call KH_fnc_triggerCbaEvent;
		};

		case "CALLBACK": {
			private _callbackArguments = _special param [1];
			private _callbackFunction = _special param [2, {}, ["", {}]];
			private _callbackId = call KH_fnc_generateUid;

			[
				"PUBLIC_VARIABLE",
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

					[_eventId] call KH_fnc_removeEventHandler;
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
					call KH_fnc_generateUid;
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

if (_special isEqualTo true) then {
	_special = ["JIP", true, false, ""];
};

if (_environment isEqualType true) exitWith {
	[_arguments, [_function, false] call KH_fnc_parseFunction, _target, _special, "", _environment] call _subfunction;
};

if !(_environment isEqualType []) then {
	_environment = [_environment];
};

private _environmentType = _environment param [0, "", [0, "", {}]];
private _environmentId = call KH_fnc_generateUid;
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
					_specialIdOverride = call KH_fnc_generateUid;
				};

				[[missionNamespace, _specialIdOverride, 2], _specialIdOverride];
			};

			case "PERSISTENT": {
				private _specialId = _special param [4, "", [""]];

				if (_specialId isNotEqualTo "") then {
					_specialIdOverride = _specialId;
				}
				else {
					_specialIdOverride = call KH_fnc_generateUid;
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
		private _unscheduled = _environment param [2, true, [true]];
		([_special, _target] call _specialParser) params ["_return", "_specialIdOverride"];
		_function = [_function, false] call KH_fnc_parseFunction;
		private "_previousReturn";
		
		if _immediate then {
			private _handlerId = [missionNamespace, _environmentId, clientOwner];
			private _totalDelta = 0;
			private _executionTime = CBA_missionTime;
			_previousReturn = [_arguments, _function, _target, _special, _specialIdOverride, _unscheduled] call _subfunction;
		};

		KH_var_temporalExecutionStackAdditions pushBack [
			[_arguments, _function, _target, _special, _subfunction, _specialIdOverride, _unscheduled, _environmentId],
			{
				params ["_arguments", "_function", "_target", "_special", "_subfunction", "_specialIdOverride", "_unscheduled", "_environmentId"];

				if !(missionNamespace getVariable _environmentId) exitWith {																											
					KH_var_temporalExecutionStackDeletions pushBackUnique _environmentId;
				};

				private _handlerId = [missionNamespace, _environmentId, clientOwner];													
				[_arguments, _function, _target, _special, _specialIdOverride, _unscheduled] call _subfunction;
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
			_environmentId,
			_environmentId,
			_previousReturn,
			CBA_missionTime
		];

		[[missionNamespace, _environmentId, clientOwner], _return];
	};

	case "CODE": {
		private _immediate = _environment param [1, true, [true]];
		private _interval = _environment param [2, 0, [0]];
		private _fireOnce = _environment param [3, true, [true]];
		private _unscheduled = _environment param [4, true, [true]];
		([_special, _target] call _specialParser) params ["_return", "_specialIdOverride"];
		_function = [_function, false] call KH_fnc_parseFunction;
		_environmentType = missionNamespace getVariable ([_environmentType, false] call KH_fnc_parseFunction);
		private _continue = true;

		if _immediate then {
			private _handlerId = [missionNamespace, _environmentId, clientOwner];
			private _executionTime = CBA_missionTime;

			if (_arguments call _environmentType) then {
				[_arguments, _function, _target, _special, _specialIdOverride, _unscheduled] call _subfunction;

				if _fireOnce then {
					_continue = false;
				};
			};
		};

		if !_continue exitWith {
			[[missionNamespace, _environmentId, clientOwner], _return];
		};

		KH_var_temporalExecutionStackAdditions pushBack [
			[_arguments, [_function, false] call KH_fnc_parseFunction, _target, _special, _subfunction, _specialIdOverride, _unscheduled, _environmentId, _environmentType, _fireOnce],
			{
				params ["_arguments", "_function", "_target", "_special", "_subfunction", "_specialIdOverride", "_unscheduled", "_environmentId", "_environmentType", "_fireOnce"];

				if !(missionNamespace getVariable _environmentId) exitWith {																											
					KH_var_temporalExecutionStackDeletions pushBackUnique _environmentId;
				};

				if (_arguments call _environmentType) then {
					private _handlerId = [missionNamespace, _environmentId, clientOwner];													
					[_arguments, _function, _target, _special, _specialIdOverride, _unscheduled] call _subfunction;

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
			_environmentId,
			_environmentId,
			_previousReturn,
			CBA_missionTime
		];

		[[missionNamespace, _environmentId, clientOwner], _return];
	};

	case "STRING": {
		if ((parseNumber _environmentType) isNotEqualTo 0) then {
			private _unscheduled = _environment param [1, true, [true]];
			([_special, _target] call _specialParser) params ["_return", "_specialIdOverride"];
			_environmentType = parseNumber _environmentType;

			KH_var_temporalExecutionStackAdditions pushBack [
				[_arguments, [_function, false] call KH_fnc_parseFunction, _target, _special, _subfunction, _specialIdOverride, _unscheduled, _environmentId],
				{
					params ["_arguments", "_function", "_target", "_special", "_subfunction", "_specialIdOverride", "_unscheduled", "_environmentId"];

					if !(missionNamespace getVariable _environmentId) exitWith {																											
						KH_var_temporalExecutionStackDeletions pushBackUnique _environmentId;
					};
													
					[_arguments, _function, _target, _special, _specialIdOverride, _unscheduled] call _subfunction;
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
				_environmentId,
				_environmentId,
				nil,
				CBA_missionTime
			];

			[[missionNamespace, _environmentId, clientOwner], _return];
		}
		else {
			switch _environmentType do {
				case "TEMPORAL": {
					private _interval = _environment param [1, 0, [0]];
					private _immediate = _environment param [2, true, [true]];
					private _conditionArguments = _environment param [3];
					private _conditionFunction = _environment param [4, {true;}, ["", {}]];
					private _timeoutRules = _environment param [5, [0, false, false, false], [true, 0, "", []]];
					private _timeoutArguments = _environment param [6];
					private _timeoutFunction = _environment param [7, {}, ["", {}]];
					private _verboseDelta = _environment param [8, false, [true]];
					private _unscheduled = _environment param [9, true, [true]];
					([_special, _target] call _specialParser) params ["_return", "_specialIdOverride"];

					[
						[
							missionNamespace, 
							_environmentId, 
							clientOwner,
							[
								[
									"TEMPORAL",
									_immediate,
									[_conditionArguments, missionNamespace getVariable ([_conditionFunction, false] call KH_fnc_parseFunction), _environmentId],
									{
										params ["_conditionArguments", "_conditionFunction", "_environmentId"];
										private _handlerId = [[missionNamespace, _environmentId, clientOwner, _eventId]];

										if (missionNamespace getVariable _environmentId) then {																											
											_conditionArguments call _conditionFunction;
										}
										else {
											[_handlerId] call KH_fnc_removeHandler;
											false;
										};
									},
									_timeoutRules,
									_timeoutArguments,
									_timeoutFunction,
									_verboseDelta
								],
								_interval,
								[_arguments, [_function, false] call KH_fnc_parseFunction, _target, _special, _subfunction, _specialIdOverride, _unscheduled, _environmentId],
								{
									params ["_arguments", "_function", "_target", "_special", "_subfunction", "_specialIdOverride", "_unscheduled", "_environmentId"];
									private _handlerId = [[missionNamespace, _environmentId, clientOwner, _eventId]];													
									[_arguments, _function, _target, _special, _specialIdOverride, _unscheduled] call _subfunction;
								}
							] call KH_fnc_addEventHandler
						], 
						_return
					];
				};

				case "SEQUENCE": {
					private _delays = _environment param [1, [], [[]]];
					private _conditionArguments = _environment param [2, [], [[]]];
					private _conditionFunctions = _environment param [3, [], [[]]];
					private _timeout = _environment param [4, 0, [0, ""]];
					private _timeoutOnDeletion = _environment param [5, false, [true]];
					private _timeoutArguments = _environment param [6];
					private _timeoutFunction = _environment param [7, {}, ["", {}]];
					private _unscheduled = _environment param [8, true, [true]];
					private _i = 0;
					private _parsedExecutionRules = [[], [], []];
					private _executions = [];
					private _return = [];
					private _totalDelay = diag_tickTime;

					for "_i" from 0 to ((count _function) - 1) do {
						private _currentArguments = _arguments param [_i];
						private _currentFunction = _function param [_i, {}, ["", {}]];
						private _currentTarget = _target param [_i, true, [true, 0, "", [], {}, objNull, teamMemberNull, grpNull, sideUnknown, locationNull]];
						private _currentSpecial = _special param [_i, false, [true, [], createHashMap]];
						private _currentDelay = _delays param [_i, 0, [0]];
						private _currentConditionArguments = _conditionArguments param [_i];
						private _currentConditionFunction = _conditionFunctions param [_i, {true;}, ["", {}]];
						_totalDelay = _totalDelay + _currentDelay;
						(_parsedExecutionRules select 0) set [_i, _totalDelay];
						(_parsedExecutionRules select 1) set [_i, _currentConditionArguments];
						(_parsedExecutionRules select 2) set [_i, missionNamespace getVariable ([_currentConditionFunction, false] call KH_fnc_parseFunction)];

						if (_currentSpecial isEqualTo true) then {
							_currentSpecial = ["JIP", true, false, ""];
						};

						if (_currentSpecial isEqualTo false) then {
							_return pushBack [];
							continue;
						};

						([_currentSpecial, _currentTarget] call _specialParser) params ["_currentReturn", "_currentSpecialIdOverride"];
						_return pushBack _currentReturn;

						_executions pushBack [
							_currentArguments, 
							[_currentFunction, false] call KH_fnc_parseFunction, 
							_currentTarget, 
							_currentSpecial, 
							_currentSpecialIdOverride,
							_unscheduled
						];
					};
					
					[
						[
							missionNamespace, 
							_environmentId,
							clientOwner,
							[
								[
									"TEMPORAL",
									true,
									[0, _parsedExecutionRules, _environmentId],
									{
										params ["_currentIndex", "_parsedExecutionRules", "_environmentId"];
										private _delay = (_parsedExecutionRules select 0) select _currentIndex;
										private _conditionArguments = (_parsedExecutionRules select 1) select _currentIndex;
										private _conditionFunction = (_parsedExecutionRules select 2) select _currentIndex;
										private _handlerId = [[missionNamespace, _environmentId, clientOwner, _eventId]];
										private _state = false;

										if (missionNamespace getVariable _environmentId) then {
											if (diag_tickTime > _delay) then {
												if (_conditionArguments call _conditionFunction) then {
													_state = true;
													_this set [0, _currentIndex + 1];
												};
											};
										}
										else {
											[_handlerId] call KH_fnc_removeHandler;
										};

										_state;
									},
									[_timeout, false, false, _timeoutOnDeletion],
									_timeoutArguments,
									_timeoutFunction,
									false
								],
								0,
								[0, _executions, _subfunction, _environmentId],
								{
									params ["_currentIndex", "_executions", "_subfunction", "_environmentId"];
									private _handlerId = [[missionNamespace, _environmentId, clientOwner, _eventId]];
									(_executions select _currentIndex) call _subfunction;
									_this set [0, _currentIndex + 1];
									
									if ((_this select 0) > ((count _executions) - 1)) then {
										[_handlerId] call KH_fnc_removeHandler;
									};
								}
							] call KH_fnc_addEventHandler
						], 
						_return
					];
				};

				default {
					nil;
				};
			};
		};
	};

	default {
		nil;
	};
};