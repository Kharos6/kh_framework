if canSuspend exitWith {
	private "_return";
	isNil {_return = ([_x] apply KH_fnc_execute) select 0;};

	if !(isNil "_return") then {
		_return;
	};
};

params [
	"_arguments", 
	["_function", {}, ["", {}]], 
	["_target", true, [true, 0, "", [], {}, objNull, teamMemberNull, grpNull, sideUnknown, locationNull]], 
	["_environment", true, [true, []]],
	["_special", false, [true, []]]
];

private _subfunction = {
	params ["_arguments", "_function", "_target", "_special", "_override"];

	if (_special isEqualTo false) exitWith {
		["KH_eve_execution", [_arguments, _function, clientOwner], _target, false] call KH_fnc_triggerCbaEvent;
	};

	switch (_special select 0) do {
		case "JIP": {
			private _dependency = _special param [1, true, [true]];
			private _unitRequired = _special param [2, false, [true]];
			private _overrideId = _special param [3, "", [""]];

			[
				"KH_eve_execution", 
				[_arguments, _function, clientOwner], 
				_target, 
				[
					_dependency, 
					_unitRequired, 
					if (_overrideId isNotEqualTo "") then {
						_overrideId;
					}
					else {
						if (_override isNotEqualTo "") then {
							_override;
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
			private _id = call KH_fnc_generateUid;
			_callbackFunction = [_callbackFunction] call KH_fnc_parseFunction;

			[
				"PUBLIC_VARIABLE",
				_id,
				[_arguments, _function],
				{
					_args params ["_arguments", "_function"];
					private _argsCallback = call KH_fnc_generateUid;
					missionNamespace setVariable [_argsCallback, _this select 1];

					_arguments call (
						compile ([
							"private _argsCallback = (missionNamespace getVariable '", _argsCallback, "');
							call ", [_function] call KH_fnc_getParsedFunction, ";"
						] joinString "")
					);

					[_localId] call KH_fnc_removeEventHandler;
				}
			] call KH_fnc_addEventHandler;

			["KH_eve_execution", [[_callbackArguments, _callbackFunction, _id, clientOwner], "KH_fnc_callback", clientOwner], _target, false] call KH_fnc_triggerCbaEvent;
		};

		case "PERSISTENT": {
			private _targetObject = _special param [1, objNull, [objNull]];

			private _persistencyFunction = [
				{
					params ["_arguments", "_function", "_targetObject", "_sendoffArguments", "_sendoffFunction", "_originalOwner", "_id"];

					[
						["STANDARD", _targetObject, "LOCAL"],
						"Local",
						[_arguments, _function, _sendoffArguments, _sendoffFunction, _originalOwner, _id],
						{
							params ["_targetObject", "_isLocal"];
							_args params ["_arguments", "_function", "_sendoffArguments", "_sendoffFunction", "_originalOwner", "_id"];
							private _idState = _targetObject getVariable [_id, "ACTIVE"];

							if (_idState isNotEqualTo "INACTIVE") then {
								switch _idState do {
									case "ACTIVE": {
										if _isLocal then {
											[_arguments, _function, _originalOwner] call KH_fnc_callParsedFunction;
										}
										else {
											[_sendoffArguments, _sendoffFunction, _originalOwner] call KH_fnc_callParsedFunction;
										};
									};

									case "TERMINATE": {
										[_localId] call KH_fnc_removeEventHandler;
									};
								};
							};
						}
					] call KH_fnc_addEventHandler;
				}
			] call KH_fnc_parseFunction;

			private _sendoffArguments = _special param [2];
			private _sendoffFunction = _special param [3, {}, ["", {}]];
			private _jip = _special param [4, true, [true]];
			_sendoffFunction = [_sendoffFunction] call KH_fnc_parseFunction;

			private _id = if (_override isNotEqualTo "") then {
				_override;
			}
			else {
				call KH_fnc_generateUid;
			};

			["KH_eve_execution", [_arguments, _function, clientOwner], _targetObject, false] call KH_fnc_triggerCbaEvent;

			[
				"KH_eve_execution",
				[
					[_arguments, _function, _targetObject, _sendoffArguments, _sendoffFunction, clientOwner, _id], 
					_persistencyFunction,
					clientOwner
				],
				"GLOBAL",
				false
			] call KH_fnc_triggerCbaEvent;

			if _jip then {
				[
					"KH_eve_execution", 
					[
						[_arguments, _function, _persistencyFunction, _targetObject, _sendoffArguments, _sendoffFunction, clientOwner, _id], 
						[
							{
								params ["_arguments", "_function", "_persistencyFunction", "_targetObject", "_sendoffArguments", "_sendoffFunction", "_originalOwner", "_id"];

								{
									[
										"CBA",
										_x,
										[_arguments, _function, _persistencyFunction, _targetObject, _sendoffArguments, _sendoffFunction, _originalOwner, _id],
										{
											_args params ["_arguments", "_function", "_persistencyFunction", "_targetObject", "_sendoffArguments", "_sendoffFunction", "_originalOwner", "_id"];
											params ["_joiningMachine"];

											[
												"KH_eve_execution",
												[
													[_arguments, _function, _targetObject, _sendoffArguments, _sendoffFunction, _originalOwner, _id], 
													_persistencyFunction,
													_originalOwner
												],
												_joiningMachine,
												false
											] call KH_fnc_triggerCbaEvent;
										}
									] call KH_fnc_addEventHandler;
								} forEach ["KH_eve_playerPreloadedInitial", "KH_eve_headlessPreloaded"];
							}
						] call KH_fnc_parseFunction,
						clientOwner
					],
					"SERVER",
					false
				] call KH_fnc_triggerCbaEvent;
			};

			["PERSISTENT_HANDLER", _id, _targetObject];
		};
	};
};

_function = [_function] call KH_fnc_parseFunction;

if (_special isEqualTo true) then {
	_special = ["JIP", true, false, ""];
};

if (_environment isEqualType true) then {
	if _environment then {
		if KH_var_missionLoaded then {			
			[_arguments, _function, _target, _special, ""] call _subfunction;
		}
		else {
			private _override = "";
			private _result = [];

			if (_special isNotEqualTo false) then {
				_override = switch (_special select 0) do {
					case "JIP": {
						private _overrideId = _special param [3, "", [""]];

						if (_overrideId isNotEqualTo "") then {
							_override = _overrideId;
						}
						else {
							_override = call KH_fnc_generateUid;
						};

						_result = ["JIP_HANDLER", _override];
					};

					case "PERSISTENT": {
						_override = call KH_fnc_generateUid;
						_result = ["PERSISTENT_HANDLER", _override, _target select 1];
					};
				};
			};

			KH_var_postInitExecutions pushBack [[_arguments, _function, _target, _special, _override], _subfunction];
			_result;
		};
	};
}
else {
	private _override = "";
	private _result = [];
	private _id = call KH_fnc_generateUid;
	
	if (_special isNotEqualTo false) then {
		if (_special isEqualTo true) then {
			_special = ["JIP", true, false, ""];
		};

		_override = switch (_special select 0) do {
			case "JIP": {
				private _overrideId = _special param [3, "", [""]];

				if (_overrideId isNotEqualTo "") then {
					_override = _overrideId;
				}
				else {
					_override = call KH_fnc_generateUid;
				};

				_result = ["JIP_HANDLER", _override];
			};

			case "PERSISTENT": {
				_override = call KH_fnc_generateUid;
				_result = ["PERSISTENT_HANDLER", _override, _target select 1];
			};
		};
	};

	switch (_environment select 0) do {
		case "TEMPORAL": {
			private _environmentCaller = {
				params [
					"_arguments", 
					"_function", 
					"_target", 
					"_special", 
					"_interval", 
					"_immediate", 
					"_conditionArguments", 
					"_conditionFunction", 
					"_timeoutRules", 
					"_timeoutArguments", 
					"_timeoutFunction", 
					"_simpleDelta", 
					"_subfunction", 
					"_override", 
					"_id"
				];

				[
					[
						"TEMPORAL",
						_immediate,
						_conditionArguments,
						_conditionFunction,
						_timeoutRules,
						_timeoutArguments,
						_timeoutFunction,
						_simpleDelta
					],
					_interval,
					[_arguments, _function, _target, _subfunction, _override, _id],
					{
						params ["_arguments", "_function", "_target", "_subfunction", "_override", "_id"];
						private _idState = missionNamespace getVariable [_id, "ACTIVE"];

						if (_idState isNotEqualTo "TERMINATE") then {
							if (_idState isEqualTo "ACTIVE") then {																											
								[_arguments, _function, _target, _special, _override] call _subfunction;
							};
						}
						else {
							[_localId] call KH_fnc_removeEventHandler;
						};
					}
				] call KH_fnc_addEventHandler;
			};

			private _interval = _environment param [1, 0, [0, ""]];
			private _immediate = _environment param [2, true, [true]];
			private _conditionArguments = _environment param [3, nil];
			private _conditionFunction = _environment param [4, {}, ["", {}]];
			private _timeoutRules = _environment param [5, [0, false, false, false], [[]]];
			private _timeoutArguments = _environment param [6, []];
			private _timeoutFunction = _environment param [7, {}, ["", {}]];
			private _simpleDelta = _environment param [8, true, [true]];
			_conditionFunction = [[_conditionFunction] call KH_fnc_parseFunction] call KH_fnc_getParsedFunction;
			_timeoutFunction = [_timeoutFunction] call KH_fnc_parseFunction;

			if KH_var_missionLoaded then {
				[
					_arguments, 
					_function, 
					_target,
					_special,
					_interval, 
					_immediate, 
					_conditionArguments, 
					_conditionFunction, 
					_timeoutRules, 
					_timeoutArguments, 
					_timeoutFunction, 
					_simpleDelta, 
					_subfunction, 
					_override, 
					_id
				] call _environmentCaller;
			}
			else {
				KH_var_postInitExecutions pushBack [
					[
						_arguments, 
						_function,
						_target,
						_special,
						_interval, 
						_immediate, 
						_conditionArguments,
						_conditionFunction,
						_timeoutRules, 
						_timeoutArguments, 
						_timeoutFunction, 
						_simpleDelta, 
						_subfunction, 
						_override, 
						_id
					],
					_environmentCaller
				];
			};

			[["PRIVATE_HANDLER", _id, clientOwner], _result];
		};

		case "SEQUENCE": {
			private _executions = _environment select 1;
			private _suspendInFrames = _environment param [2, false];
			private _overrides = [];
			private _results = [];
			private _newExecutions = call KH_fnc_generateUid;
			private _newExecutionEvent = call KH_fnc_generateUid;
			private _sequenceArguments = call KH_fnc_generateUid;

			{
				private _currentOverride = "";
				private _currentResult = true;
				private _currentTarget = (_x select 1) param [2, _target];

				if (_currentTarget isEqualType []) then {
					private _type = _currentTarget select 0;

					switch _type do {
						case "JIP": {
							if (_forEachIndex == 0) then {
								private _overrideId = _currentTarget param [5, ""];

								if (_overrideId isNotEqualTo "") then {
									_currentOverride = _overrideId;
								}
								else {
									_currentOverride = _override;
								};

								_currentResult = ["JIP_HANDLER", _currentOverride];
							}
							else {
								private _overrideId = _currentTarget param [5, ""];

								if (_overrideId isNotEqualTo "") then {
									_currentOverride = _overrideId;
								}
								else {
									_currentOverride = call KH_fnc_generateUid;
								};

								_currentResult = ["JIP_HANDLER", _currentOverride];
							};
						};

						case "PERSISTENT": {
							if (_forEachIndex == 0) then {
								_currentOverride = _override;
							}
							else {
								_currentOverride = call KH_fnc_generateUid;
							};

							_currentResult = ["PERSISTENT_HANDLER", _currentOverride, _target select 1];
						};
					};
				};

				_overrides pushBack _currentOverride;
				_results pushBack _currentResult;
			} forEach _executions;

			private _environmentCaller = {
				params [
					"_arguments", 
					"_function", 
					"_target", 
					"_environment", 
					"_executions", 
					"_id", 
					"_overrides", 
					"_newExecutions", 
					"_newExecutionEvent", 
					"_suspendInFrames", 
					"_sequenceArguments"
				];
				private _timeSuspensions = [0];
				private _conditionSuspensions = [[[], {true;}, true, -1, [], {}, _target, _environment, call KH_fnc_generateUid]];
				
				{
					(_x select 0) params [
						["_delay", 0], 
						["_conditionArguments", []], 
						["_conditionFunction", {true;}], 
						["_timeout", -1], 
						["_timeoutArguments", []],
						["_timeoutFunction", {}], 
						["_timeoutTarget", _target], 
						["_timeoutEnvironment", _environment]
					];

					(_x select 1) params [
						["_currentArguments", _arguments], 
						["_currentFunction", _function], 
						["_currentTarget", _target], 
						["_currentEnvironment", _environment]
					];

					if ((_delay isEqualType "") && !_suspendInFrames) then {
						_delay = ((parseNumber _delay) - CBA_missionTime) max 0;
					};

					_timeSuspensions pushBack _delay;
					_conditionSuspensions pushBack [
						_conditionArguments, 
						[[_conditionFunction] call KH_fnc_parseFunction] call KH_fnc_getParsedFunction, 
						_evaluatePrevious, 
						_timeout, 
						_timeoutArguments, 
						_timeoutFunction, 
						_timeoutTarget, 
						_timeoutEnvironment, 
						call KH_fnc_generateUid
					];

					private _currentExecutionId = call KH_fnc_generateUid;
					private _conditionId = call KH_fnc_generateUid;

					[
						{
							private _id = _this select 8;
							private _idState = missionNamespace getVariable [_id, "ACTIVE"];
							private _currentExecutionId = _this select 12;
							private _condition = true;

							switch _idState do {
								case "ACTIVE": {
									private _conditionSuspensions = _this select 6;
									private _conditionId = _this select 7;
									private _sequenceArguments = _this select 13;

									if ((_conditionSuspensions select -1) select 2) then {												
										{
											_x params ["_conditionArguments", "_conditionFunction"];

											private _expression = [
												"private _args = missionNamespace getVariable ['", _sequenceArguments, "', []];
												call ", _conditionFunction, ";"
											] joinString "";

											if !(_conditionArguments call (compile _expression)) then {
												if !(_x in (missionNamespace getVariable [_conditionId, []])) then {
													_condition = false;
													break;
												};
											}
											else {
												if ((_conditionSuspensions select -1) isNotEqualTo _x) then {
													private _currentConditions = missionNamespace getVariable [_conditionId, []];
													_currentConditions pushBack _x;
													missionNamespace setVariable [_conditionId, _currentConditions];
												};
											};
										} forEach _conditionSuspensions;
									}
									else {
										(_conditionSuspensions select -1) params ["_conditionArguments", "_conditionFunction"];

										private _expression = [
											"private _args = missionNamespace getVariable ['", _sequenceArguments, "', []];
											call ", _conditionFunction, ";"
										] joinString "";

										if !(_conditionArguments call (compile _expression)) then {
											_condition = false;
										};
									};
								};

								case "INACTIVE": {
									_condition = false;
								};

								case "TERMINATE": {
									_condition = true;
								};		
							};

							if !(missionNamespace getVariable [_currentExecutionId, false]) then {
								_condition = false;
							};

							_condition;
						}, 
						{
							params ["_currentArguments", "_currentFunction", "_currentTarget", "_currentEnvironment", "_environment", "_subfunction", "_conditionSuspensions"];
							private _id = _this select 8;
							private _idState = missionNamespace getVariable [_id, "ACTIVE"];

							if (_idState isEqualTo "ACTIVE") then {
								private _overrides = _this select 9;
								private _newExecutions = _this select 10;
								private _newExecutionEvent = _this select 11;
								private _sequenceArguments = _this select 13;
								_currentFunction = [[_currentFunction] call KH_fnc_parseFunction] call KH_fnc_getParsedFunction;

								private _expression = [
									"private _args = missionNamespace getVariable ['", _sequenceArguments, "', []];
									private _sequenceResult = call ", _currentFunction, ";
									missionNamespace setVariable ['", _sequenceArguments, "', _sequenceResult];"
								] joinString "";
							
								if (_currentEnvironment isEqualTo _environment) then {
									[_currentArguments, compile _expression, _currentTarget, _overrides select ((count _conditionSuspensions) - 2)] call _subfunction;
								}
								else {
									private _currentExecutions = missionNamespace getVariable [_newExecutions, []];
									private _newExecution = [_currentArguments, compile _expression, _currentTarget, _currentEnvironment] call KH_fnc_execute;
									_currentExecutions pushBack _newExecution;
									missionNamespace setVariable [_newExecutions, _currentExecutions];
									[_newExecutionEvent, [_newExecution, _currentExecutions, missionNamespace getVariable [_sequenceArguments, []]]] call CBA_fnc_localEvent;
								};
							};
						}, 
						[
							_currentArguments, 
							_currentFunction, 
							_currentTarget, 
							_currentEnvironment, 
							_environment, 
							_subfunction, 
							_conditionSuspensions, 
							_conditionId, 
							_id, 
							_overrides, 
							_newExecutions, 
							_newExecutionEvent, 
							_currentExecutionId, 
							_sequenceArguments
						],
						(_conditionSuspensions select -1) select 3,
						{
							private _id = _this select 8;
							private _idState = missionNamespace getVariable [_id, "ACTIVE"];

							if (_idState isEqualTo "ACTIVE") then {
								private _environment = _this select 4;
								private _subfunction = _this select 5;
								private _overrides = _this select 9;
								private _newExecutions = _this select 10;
								private _newExecutionEvent = _this select 11;
								private _sequenceArguments = _this select 13;
								private _timeoutArguments = ((_this select 6) select -1) select 4;
								private _timeoutFunction = ((_this select 6) select -1) select 5;
								private _timeoutTarget = ((_this select 6) select -1) select 6;
								private _timeoutEnvironment = ((_this select 6) select -1) select 7;
								_timeoutFunction = [[_timeoutFunction] call KH_fnc_parseFunction] call KH_fnc_getParsedFunction;

								private _expression = [
									"private _args = missionNamespace getVariable ['", _sequenceArguments, "', []];
									private _sequenceResult = call ", _timeoutFunction, ";
									missionNamespace setVariable ['", _sequenceArguments, "', _sequenceResult];"
								] joinString "";

								if (_timeoutEnvironment isEqualTo _environment) then {
									[_timeoutArguments, compile _expression, _timeoutTarget, _overrides select ((count _conditionSuspensions) - 2)] call _subfunction;
								}
								else {
									private _currentExecutions = missionNamespace getVariable [_newExecutions, []];
									private _newExecution = [_timeoutArguments, compile _expression, _timeoutTarget, _timeoutEnvironment] call KH_fnc_execute;
									_currentExecutions pushBack _newExecution;
									missionNamespace setVariable [_newExecutions, _currentExecutions];
									[_newExecutionEvent, [_newExecution, _currentExecutions, missionNamespace getVariable [_sequenceArguments, []]]] call CBA_fnc_localEvent;
								};
							};
						}
					] call CBA_fnc_waitUntilAndExecute;

					if _suspendInFrames then {
						[
							{
								params ["_currentExecutionId"];
								missionNamespace setVariable [_currentExecutionId, true];
							}, 
							[_currentExecutionId],
							[_timeSuspensions] call KH_fnc_arraySum
						] call CBA_fnc_execAfterNFrames;
					}
					else {
						[
							{
								params ["_currentExecutionId"];
								missionNamespace setVariable [_currentExecutionId, true];
							}, 
							[_currentExecutionId],
							[_timeSuspensions] call KH_fnc_arraySum
						] call CBA_fnc_waitAndExecute;
					};
				} forEach _executions;
			};

			if KH_var_missionLoaded then {
				[
					_arguments, 
					_function, 
					_target, 
					true, 
					_executions, 
					_id, 
					_overrides, 
					_newExecutions, 
					_newExecutionEvent, 
					_suspendInFrames, 
					_sequenceArguments
				] call _environmentCaller;
			}
			else {
				KH_var_postInitExecutions pushBack [
					[
						_arguments, 
						_function, 
						_target, 
						true, 
						_executions,
						_id, 
						_overrides, 
						_newExecutions, 
						_newExecutionEvent, 
						_suspendInFrames, 
						_sequenceArguments
					],
					_environmentCaller
				];
			};

			[["PRIVATE_HANDLER", _id, clientOwner], _results, _newExecutions, _newExecutionEvent, _sequenceArguments];
		};
	};
};