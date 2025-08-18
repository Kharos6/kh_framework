if canSuspend exitWith {
	private "_return";
	isNil {_return = ([_x] apply KH_fnc_execute) select 0;};

	if !(isNil "_return") then {
		_return;
	};
};

params [
	["_arguments", nil], 
	["_function", {}, ["", [], {}]], 
	["_target", true, [true, 0, "", [], {}, objNull, teamMemberNull, grpNull, sideUnknown, locationNull]], 
	["_environment", true, [true, []]],
	["_special", false, [true, []]]
];

private _subfunction = {
	params [
		["_arguments", nil], 
		["_function", {}, ["", {}]], 
		["_target", true, [true, 0, "", [], {}, objNull, teamMemberNull, grpNull, sideUnknown, locationNull]], 
		["_special", false, [true, []]], 
		["_override", "", [""]]
	];

	if (_special isEqualTo false) exitWith {
		["KH_eve_execution", [_arguments, _function, clientOwner], _target, false] call KH_fnc_triggerCbaEvent;
	};

	switch (_special select 0) do {
		case "JIP": {
			private _dependency = _special param [1, true, [true, 0, "", [], {}, objNull, teamMemberNull, grpNull]];
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
			private _callbackArguments = _special param [1, nil];
			private _callbackFunction = _special param [2, {}, ["", {}]];
			private _id = call KH_fnc_generateUid;
			_callbackFunction = [_callbackFunction, false] call KH_fnc_parseFunction;

			[
				"PUBLIC_VARIABLE",
				_id,
				[_arguments, _function],
				{
					_args params ["_arguments", "_function"];
					private _argsCallback = param [1, nil];
					_arguments call (missionNamespace getVariable _function);
					[_localId] call KH_fnc_removeEventHandler;
				}
			] call KH_fnc_addEventHandler;

			["KH_eve_execution", [[_callbackArguments, _callbackFunction, _id, clientOwner], "KH_fnc_registerCallback", clientOwner], _target, false] call KH_fnc_triggerCbaEvent;
		};

		case "PERSISTENT": {
			private _sendoffArguments = _special param [1, nil];
			private _sendoffFunction = _special param [2, {}, ["", {}]];
			private _jip = _special param [3, true, [true]];
			_sendoffFunction = [_sendoffFunction, false] call KH_fnc_parseFunction;

			private _id = if (_override isNotEqualTo "") then {
				_override;
			}
			else {
				call KH_fnc_generateUid;
			};

			["KH_eve_execution", [_arguments, _function, clientOwner], _target, false] call KH_fnc_triggerCbaEvent;

			[
				"KH_eve_execution",
				[[_arguments, _function, _target, _sendoffArguments, _sendoffFunction, clientOwner, _id], "KH_fnc_persistentExecution", clientOwner],
				"GLOBAL",
				if _jip then {
					[_target, false, _id];
				}
				else {
					false;
				}
			] call KH_fnc_triggerCbaEvent;

			["PERSISTENT_HANDLER", _id, _target];
		};
	};
};

if (_special isEqualTo true) then {
	_special = ["JIP", true, false, ""];
};

if (_environment isEqualType true) then {
	if _environment then {
		_function = [_function, false] call KH_fnc_parseFunction;

		if KH_var_missionLoaded then {			
			[_arguments, _function, _target, _special, ""] call _subfunction;
		}
		else {
			private _override = "";
			private "_result";

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
						_result = ["PERSISTENT_HANDLER", _override, _target];
					};
				};
			};

			KH_var_postInitExecutions pushBack [[_arguments, _function, _target, _special, _override], _subfunction];

			if !(isNil "_result") then {
				_result;
			};
		};
	};
}
else {
	private _id = call KH_fnc_generateUid;
	
	switch (_environment select 0) do {
		case "TEMPORAL": {
			_function = [_function, false] call KH_fnc_parseFunction;
			private _interval = _environment param [1, 0, [0, ""]];
			private _immediate = _environment param [2, true, [true]];
			private _conditionArguments = _environment param [3, nil];
			private _conditionFunction = _environment param [4, {}, ["", {}]];
			private _timeoutRules = _environment param [5, [0, false, false, false], [[]]];
			private _timeoutArguments = _environment param [6, []];
			private _timeoutFunction = _environment param [7, {}, ["", {}]];
			private _simpleDelta = _environment param [8, true, [true]];
			_conditionFunction = [[_conditionFunction, false] call KH_fnc_parseFunction] call KH_fnc_getParsedFunction;
			_timeoutFunction = [_timeoutFunction, false] call KH_fnc_parseFunction;
			private _override = "";
			private "_result";
			
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
						_result = ["PERSISTENT_HANDLER", _override, _target];
					};
				};
			};
			
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
						[_conditionArguments, _conditionFunction, _id],
						{
							params ["_conditionArguments", "_conditionFunction", "_id"];
							private _idState = missionNamespace getVariable [_id, "ACTIVE"];

							if (_idState isNotEqualTo "TERMINATE") then {
								if (_idState isEqualTo "ACTIVE") then {																											
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
						_simpleDelta
					],
					_interval,
					[_arguments, _function, _target, _special, _subfunction, _override],
					{
						params ["_arguments", "_function", "_target", "_special", "_subfunction", "_override"];																
						[_arguments, _function, _target, _special, _override] call _subfunction;
					}
				] call KH_fnc_addEventHandler;
			};

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
			private _environmentCaller = {
				params ["_executions", "_executionRules", "_timeout", "_timeoutOnDeletion", "_timeoutArguments", "_timeoutFunction", "_subfunction", "_id"];

				[
					[
						"TEMPORAL",
						true,
						[0, _executionRules, _id],
						{
							params ["_currentIndex", "_executionRules", "_id"];
							(_executionRules select _currentIndex) params ["_suspension", "_conditionArguments", "_conditionFunction"];
							private _idState = missionNamespace getVariable [_id, "ACTIVE"];
							private _state = false;

							if (_idState isNotEqualTo "TERMINATE") then {
								if (_idState isEqualTo "ACTIVE") then {
									if (diag_tickTime > _suspension) then {
										if (_conditionArguments call _conditionFunction) then {
											_state = true;
											_this set [0, (_currentIndex + 1) min ((count _executionRules) - 1)];
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
						true
					],
					0,
					[0, _executions, _subfunction],
					{
						_args params ["_currentIndex", "_executions", "_subfunction"];
						(_executions select _currentIndex) call _subfunction;
						_args set [0, (_currentIndex + 1) min ((count _executions) - 1)];
					}
				] call KH_fnc_addEventHandler;
			};

			private _executionRules = _environment param [1, [], [[]]];
			private _timeout = _environment param [2, 0, [0]];
			private _timeoutOnDeletion = _environment param [3, false, [true]];
			private _timeoutArguments = _environment param [3, nil];
			private _timeoutFunction = _environment param [4, {}, ["", {}]];
			private _i = 0;
			private _executions = [];
			private _results = [];

			for "_i" from 0 to ((count _function) - 1) do {
				private _override = call KH_fnc_generateUid;

				_executions pushBack [
					_arguments param [_i, nil], 
					[_functions param [_i, {}, ["", {}]], false] call KH_fnc_parseFunction, 
					_target param [_i, true, [true, 0, "", [], {}, objNull, teamMemberNull, grpNull, sideUnknown, locationNull]], 
					_special param [_i, false, [true, []]], 
					_override
				];

				switch (_special select _i) do {
					case "JIP": {
						_results pushBack ["JIP_HANDLER", _override];
					};

					case "PERSISTENT": {
						_results pushBack ["PERSISTENT_HANDLER", _override, _target select _i];
					};
				};
			};

			{
				_x set [0, diag_tickTime + ((_x select [0, _forEachIndex + 1]) call KH_fnc_arraySum)];
				_x set [1, _x param [1, nil]];
				_x set [2, [[_x param [2, {true;}, ["", {}]], false] call KH_fnc_parseFunction] call KH_fnc_getParsedFunction];
			} forEach _executionRules;

			if KH_var_missionLoaded then {
				[
					_executions, 
					_executionRules,
					_timeout,
					_timeoutOnDeletion,
					_timeoutArguments,
					_timeoutFunction,
					_subfunction,
					_id
				] call _environmentCaller;
			}
			else {
				KH_var_postInitExecutions pushBack [
					[
						_executions, 
						_executionRules,
						_timeout,
						_timeoutOnDeletion,
						_timeoutArguments,
						_timeoutFunction,
						_subfunction,
						_id
					],
					_environmentCaller
				];
			};

			[["PRIVATE_HANDLER", _id, clientOwner], _results];
		};
	};
};