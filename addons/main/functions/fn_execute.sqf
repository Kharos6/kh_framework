if canSuspend exitWith {
	private "_return";
	isNil {_return = ([_x] apply KH_fnc_execute) select 0;};

	if !(isNil "_return") then {
		_return;
	};
};

params [
	"_arguments", 
	["_function", {}, ["", [], {}]], 
	["_target", true, [true, 0, "", [], {}, objNull, teamMemberNull, grpNull, sideUnknown, locationNull]], 
	["_environment", true, [true, []]],
	["_special", false, [true, [], createHashMap]]
];

private _subfunction = {
	params ["_arguments", "_function", "_target", "_special", "_specialIdOverride"];

	if (_special isEqualTo false) exitWith {
		["KH_eve_execution", [_arguments, _function, clientOwner], _target, false] call KH_fnc_triggerCbaEvent;
	};

	if (_special isEqualType createHashMap) exitWith {
		private _argumentsId = call KH_fnc_generateUid;
		missionNamespace setVariable [_argumentsId, _arguments];

		[
			"KH_eve_execution", 
			[
				_arguments, 
				[compile ([_special, " call ['", _function, "', missionNamespace getVariable '", _argumentsId, "'];"] joinString "")] call KH_fnc_parseFunction, 
				clientOwner
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
				[_arguments, _function, clientOwner], 
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
				[_arguments, _function],
				{
					_args params ["_arguments", "_function"];
					private _argsCallback = param [1];
					_arguments call (missionNamespace getVariable _function);
					[_localId] call KH_fnc_removeEventHandler;
				}
			] call KH_fnc_addEventHandler;

			["KH_eve_registerCallback", [_callbackArguments, [_callbackFunction, false] call KH_fnc_parseFunction, clientOwner, _callbackId], _target, false] call KH_fnc_triggerCbaEvent;
		};

		case "PERSISTENT": {
			private _sendoffArguments = _special param [1];
			private _sendoffFunction = _special param [2, {}, ["", {}]];
			private _persistencyId = _special param [3, "", [""]];

			if (_persistencyId isNotEqualTo "") then {
				_persistencyId;
			}
			else {
				if (_specialIdOverride isNotEqualTo "") then {
					_specialIdOverride;
				}
				else {
					call KH_fnc_generateUid;
				};
			};

			_target setVariable [_persistencyId, "ACTIVE", true];
			private _persistencyEventId = [hashValue _target, _persistencyId] joinString "";
			private _continue = true;

			if !(isNil {missionNamespace getVariable _persistencyEventId;}) then {
				_continue = false;
			};

			missionNamespace setVariable [_persistencyEventId, [_arguments, _function, _sendoffArguments, [_sendoffFunction, false] call KH_fnc_parseFunction, clientOwner], true];	
			["KH_eve_execution", [_arguments, _function, clientOwner], _target, false] call KH_fnc_triggerCbaEvent;
			
			if !_continue exitWith {
				[_target, _persistencyId, true];
			};

			[
				"KH_eve_execution",
				[
					[
						["STANDARD", _target, "LOCAL"],
						"Local",
						[_persistencyId, _persistencyEventId],
						{
							params ["_entity", "_local"];
							_args params ["_persistencyId", "_persistencyEventId"];
							private _persistencyState = _entity getVariable _persistencyId;

							if (_persistencyState isNotEqualTo "TERMINATE") then {
								if (_persistencyState isEqualTo "ACTIVE") then {
									(missionNamespace getVariable _persistencyEventId) params ["_arguments", "_function", "_sendoffArguments", "_sendoffFunction", "_caller"];
									
									if _local then {
										[_arguments, _function, _caller] call KH_fnc_callParsedFunction;
									}
									else {
										[_sendoffArguments, _sendoffFunction, _caller] call KH_fnc_callParsedFunction;
									};
								};
							}
							else {
								[_localId] call KH_fnc_removeEventHandler;
							};
						}
					], 
					"KH_fnc_addEventHandler", 
					clientOwner
				],
				"GLOBAL",
				[_target, false, _persistencyId]
			] call KH_fnc_triggerCbaEvent;

			[_target, _persistencyId, true];
		};
	};
};

if (_special isEqualTo true) then {
	_special = ["JIP", true, false, ""];
};

if (_environment isEqualType true) exitWith {
	if _environment then {	
		[_arguments, [_function, false] call KH_fnc_parseFunction, _target, _special, ""] call _subfunction;
	};
};

private _environmentId = call KH_fnc_generateUid;
missionNamespace setVariable [_environmentId, "ACTIVE"];
private _environmentType = _environment param [0, "", [""]];

switch _environmentType do {
	case "TEMPORAL": {
		private _interval = _environment param [1, 0, [0, ""]];
		private _immediate = _environment param [2, true, [true]];
		private _conditionArguments = _environment param [3];
		private _conditionFunction = _environment param [4, {true;}, ["", {}]];
		private _timeoutRules = _environment param [5, [0, false, false, false], [true, 0, "", []]];
		private _timeoutArguments = _environment param [6];
		private _timeoutFunction = _environment param [7, {}, ["", {}]];
		private _verboseDelta = _environment param [8, false, [true]];
		private _specialIdOverride = "";
		private "_return";
		
		if ((_special isNotEqualTo false) && !(_special isEqualType createHashMap)) then {
			private _specialType = _special param [0, "", [""]];

			switch _specialType do {
				case "JIP": {
					private _specialId = _special param [3, "", [""]];

					if (_specialId isNotEqualTo "") then {
						_specialIdOverride = _specialId;
					}
					else {
						_specialIdOverride = call KH_fnc_generateUid;
					};

					_return = [missionNamespace, _specialIdOverride, 2];
				};

				case "PERSISTENT": {
					private _specialId = _special param [3, "", [""]];

					if (_specialId isNotEqualTo "") then {
						_specialIdOverride = _specialId;
					}
					else {
						_specialIdOverride = call KH_fnc_generateUid;
					};

					_return = [_target, _specialIdOverride, true];
				};
			};
		};

		[
			[
				"TEMPORAL",
				_immediate,
				[_conditionArguments, missionNamespace getVariable ([_conditionFunction, false] call KH_fnc_parseFunction), _environmentId],
				{
					params ["_conditionArguments", "_conditionFunction", "_environmentId"];
					private _environmentState = missionNamespace getVariable _environmentId;

					if (_environmentState isNotEqualTo "TERMINATE") then {
						if (_environmentState isEqualTo "ACTIVE") then {																											
							_conditionArguments call _conditionFunction;
						}
						else {
							false;
						};
					}
					else {
						[_localId] call KH_fnc_removeEventHandler;
						false;
					};
				},
				_timeoutRules,
				_timeoutArguments,
				_timeoutFunction,
				_verboseDelta
			],
			_interval,
			[_arguments, [_function, false] call KH_fnc_parseFunction, _target, _special, _subfunction, _specialIdOverride],
			{
				params ["_arguments", "_function", "_target", "_special", "_subfunction", "_specialIdOverride"];																
				[_arguments, _function, _target, _special, _specialIdOverride] call _subfunction;
			}
		] call KH_fnc_addEventHandler;

		[[missionNamespace, _environmentId, clientOwner], _return];
	};

	case "SEQUENCE": {
		private _delays = _environment param [1, [], [[]]];
		private _conditionArguments = _environment param [2, [], [[]]];
		private _conditionFunctions = _environment param [3, [], [[]]];
		private _timeout = _environment param [4, 0, [0]];
		private _timeoutOnDeletion = _environment param [5, false, [true]];
		private _timeoutArguments = _environment param [6];
		private _timeoutFunction = _environment param [7, {}, ["", {}]];
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
			private _currentSpecialIdOverride = "";

			if (_currentSpecial isEqualTo true) then {
				_currentSpecial = ["JIP", true, false, ""];
			};

			if (_currentSpecial isEqualTo false) then {
				_return pushBack [];
				continue;
			};

			if !(_currentSpecial isEqualType createHashMap) then {
				private _currentSpecialType = _currentSpecial param [0, "", [""]];

				switch _currentSpecialType do {
					case "JIP": {
						private _currentSpecialId = _currentSpecial param [3, "", [""]];
						
						if (_currentSpecialId isNotEqualTo "") then {
							_currentSpecialIdOverride = _currentSpecialId;
						}
						else {
							_currentSpecialIdOverride = call KH_fnc_generateUid;
						};

						_return pushBack [missionNamespace, _currentSpecialIdOverride, 2];
					};

					case "PERSISTENT": {
						private _currentSpecialId = _currentSpecial param [3, "", [""]];
						
						if (_currentSpecialId isNotEqualTo "") then {
							_currentSpecialIdOverride = _currentSpecialId;
						}
						else {
							_currentSpecialIdOverride = call KH_fnc_generateUid;
						};

						_return pushBack [_currentTarget, _currentSpecialIdOverride, true];
					};

					default {
						_return pushBack [];
					};
				};
			}
			else {
				_return pushBack [];
			};

			_executions pushBack [
				_currentArguments, 
				[_currentFunction, false] call KH_fnc_parseFunction, 
				_currentTarget, 
				_currentSpecial, 
				_currentSpecialIdOverride
			];
		};

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
					private _environmentState = missionNamespace getVariable _environmentId;
					private _state = false;

					if (_environmentState isNotEqualTo "TERMINATE") then {
						if (_environmentState isEqualTo "ACTIVE") then {
							if (diag_tickTime > _delay) then {
								if (_conditionArguments call _conditionFunction) then {
									_state = true;
									_this set [0, _currentIndex + 1];
								};
							};
						};
					}
					else {
						[_localId] call KH_fnc_removeEventHandler;
						_state = false;
					};

					_state;
				},
				[_timeout, false, false, _timeoutOnDeletion],
				_timeoutArguments,
				_timeoutFunction,
				false
			],
			0,
			[0, _executions, _subfunction],
			{
				params ["_currentIndex", "_executions", "_subfunction"];
				(_executions select _currentIndex) call _subfunction;
				_this set [0, _currentIndex + 1];
				
				if ((_this select 0) > ((count _executions) - 1)) then {
					[_localId] call KH_fnc_removeEventHandler;
				};
			}
		] call KH_fnc_addEventHandler;
		
		[[missionNamespace, _environmentId, clientOwner], _return];
	};
};