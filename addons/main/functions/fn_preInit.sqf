KH_var_khDataNamespace = ["kh_namespace_", profileName] joinString "";
uiNamespace setVariable ["KH_var_khDataNamespace", KH_var_khDataNamespace];
KH_var_isMenu = allDisplays isEqualTo [findDisplay 0];
if KH_var_isMenu exitWith {};
KH_var_gameSessionId = uiNamespace getVariable "KH_var_gameSessionId";
KH_var_missionSessionId = generateUid;
uiNamespace setVariable ["KH_var_missionSessionId", KH_var_missionSessionId];
KH_var_aceLoaded = uiNamespace getVariable "KH_var_aceLoaded";
KH_var_missionLoaded = false;
KH_var_jip = false;
KH_var_playerUnit = objNull;
KH_var_defaultAiSystemPrompt = uiNamespace getVariable "KH_var_defaultAiSystemPrompt";
KH_var_remoteExecCommandsBlacklist = createHashMap;
KH_var_remoteExecFunctionsBlacklist = createHashMap;
KH_var_remoteExecCommandsJipBlacklist = createHashMap;
KH_var_remoteExecFunctionsJipBlacklist = createHashMap;
KH_var_inGameUiEventHandlerStack = createHashMap;
KH_var_temporalExecutionStackMonitor = createHashMap;
KH_var_drawUiExecutionStackMonitor = createHashMap;
KH_var_quickFunctionsSqf = createHashMap;
uiNamespace setVariable ["KH_var_quickFunctionsSqf", KH_var_quickFunctionsSqf];
KH_var_quickFunctionsLua = createHashMap;
uiNamespace setVariable ["KH_var_quickFunctionsLua", KH_var_quickFunctionsLua];
KH_var_inGameUiEventHandlerStackDeletions = [];
KH_var_temporalExecutionStack = [];
KH_var_drawUi2dExecutionStack = [];
KH_var_drawUi3dExecutionStack = [];
KH_var_drawUi3dOrphanExecutionStack = [];
KH_var_temporalExecutionStackAdditions = [];
KH_var_drawUi2dExecutionStackAdditions = [];
KH_var_drawUi3dExecutionStackAdditions = [];
KH_var_temporalExecutionStackDeletions = [];
KH_var_drawUi2dExecutionStackDeletions = [];
KH_var_drawUi3dExecutionStackDeletions = [];
KH_var_postInitExecutions = [];
KH_var_preInitLuaExecutions = [];
KH_var_postInitLuaExecutions = [];
KH_var_entityInitializations = [];
KH_var_entityInitializationsDeletions = [];
KH_var_mouseTargetCheckFrame = 0;
KH_var_viewTargetCheckFrame = 0;
KH_var_weaponTargetCheckFrame = 0;
KH_var_allAddedDisplays = [];
KH_var_playerRespawnedEventHandler = [];
KH_var_playerKilledEventHandler = [];
["CBA", "KH_eve_execution", [], KH_fnc_callSerializedFunction] call KH_fnc_addEventHandler;

[
	"CBA",
	"KH_eve_luaEventTrigger",
	[],
	{
		params [["_event", "", [""]], "_arguments"];
		luaTriggerEvent _event;
	}
] call KH_fnc_addEventHandler;

[
	"CBA",
	"KH_eve_luaVariableEmission",
	[],
	{
		params [["_name", "", [""]], "_value"];
		luaSetVariable [_name, _value];
	}
] call KH_fnc_addEventHandler;

[
	"CBA",
	"KH_eve_khDataWriteEmission", 
	[],
	{
		params [["_filename", "", [""]], ["_name", "", [""]], "_value"];
		_filename writeKhData [_name, _value];
	}
] call KH_fnc_addEventHandler;

{
    private _prefix = getText (_x >> "prefix");
    private _basePath = (getText (_x >> "path")) regexReplace ["(/)", "\\"];

    {
        private _function = preprocessFile ([
            _basePath,
            ["", "\"] select (_basePath isNotEqualTo ""),
            if (isText (_x >> "path")) then {
                (getText (_x >> "path")) regexReplace ["(/)", "\\"];
            }
            else {
                "";
            }, 
            configName _x, 
            ".lua"
        ] joinString "");

        private _name = if (isText (_x >> "name")) then {
            [_prefix, getText (_x >> "name")] joinString "_";
        }
        else {
           	[_prefix, configName _x] joinString "_";
        };

		if (isNumber (_x >> "compileSqf")) then {
			if ((getNumber (_x >> "compileSqf")) isEqualTo 1) then {
				private _compiled = [
                    [
                        _prefix,
                        "_fnc_",
                        if (isText (_x >> "name")) then {
                            getText (_x >> "name");
                        }
                        else {
                            configName _x;
                        }
                    ] joinString "",
                    compile (["_this luaExecute '", _name, "';"] joinString "")
				];

                missionNamespace setVariable _compiled;
				uiNamespace setVariable _compiled;
            };
        };

        _name luaCompile _function;

		if (isNumber (_x >> "preInit")) then {
			if ((getNumber (_x >> "preInit")) isEqualTo 1) then {
				if !(_name in KH_var_preInitLuaExecutions) then {
        			KH_var_preInitLuaExecutions pushBack _name;
                };
			};
        };

		if (isNumber (_x >> "postInit")) then {
			if ((getNumber (_x >> "postInit")) isEqualTo 1) then {
				if !(_name in KH_var_postInitLuaExecutions) then {
        			KH_var_postInitLuaExecutions pushBack _name;
                };
			};
        };
    } forEach ("true" configClasses _x);
} forEach (("true" configClasses (configFile >> "CfgLuaFunctions")) + ("true" configClasses (missionConfigFile >> "CfgLuaFunctions")));

[
	"CBA",
	"KH_eve_luaReset",
	[],
	{
		with uiNamespace do {
			{
				private _prefix = getText (_x >> "prefix");
				private _basePath = (getText (_x >> "path")) regexReplace ["(/)", "\\"];

				{
					private _function = preprocessFile ([
						_basePath,
						["", "\"] select (_basePath isNotEqualTo ""),
						if (isText (_x >> "path")) then {
							(getText (_x >> "path")) regexReplace ["(/)", "\\"];
						}
						else {
							"";
						}, 
						configName _x, 
						".lua"
					] joinString "");

					private _name = if (isText (_x >> "name")) then {
						[_prefix, getText (_x >> "name")] joinString "_";
					}
					else {
						[_prefix, configName _x] joinString "_";
					};

					_name luaCompile _function;

					{
						luaExecute _x;
					} forEach KH_var_resetInitLuaExecutions;
				} forEach ("true" configClasses _x);
			} forEach ("true" configClasses (configFile >> "CfgLuaFunctions"));
		};
	}
] call KH_fnc_addEventHandler;

KH_var_remoteExecCommandsMode = ["SCALAR", ["'CfgRemoteExec' >> 'Commands' >> 'mode'", true]] call KH_fnc_getConfigValue;
KH_var_remoteExecFunctionsMode = ["SCALAR", ["'CfgRemoteExec' >> 'Functions' >> 'mode'", true]] call KH_fnc_getConfigValue;
KH_var_remoteExecCommandsJipMode = ["SCALAR", ["'CfgRemoteExec' >> 'Commands' >> 'jip'", true]] call KH_fnc_getConfigValue;
KH_var_remoteExecFunctionsJipMode = ["SCALAR", ["'CfgRemoteExec' >> 'Functions' >> 'jip'", true]] call KH_fnc_getConfigValue;

if (KH_var_remoteExecCommandsMode isEqualTo 1) then {
	{
		if (isNumber (_x >> "allowedTargets")) then {
			if ((getNumber (_x >> "allowedTargets")) isNotEqualTo 0) then {
				KH_var_remoteExecCommandspBlacklist set [toLowerANSI (configName _x), true];
			};
		};

		if (isNumber (_x >> "jip")) then {
			if ((getNumber (_x >> "jip")) isEqualTo 0) then {
				KH_var_remoteExecCommandsJipBlacklist set [toLowerANSI (configName _x), true];
			};
		};
	} forEach ("true" configClasses (configFile >> "CfgRemoteExec" >> "Commands"));

	{
		if (isNumber (_x >> "allowedTargets")) then {
			if ((getNumber (_x >> "allowedTargets")) isNotEqualTo 0) then {
				KH_var_remoteExecCommandspBlacklist set [toLowerANSI (configName _x), true, true];
			};
		};

		if (isNumber (_x >> "jip")) then {
			if ((getNumber (_x >> "jip")) isEqualTo 0) then {
				KH_var_remoteExecCommandsJippBlacklist set [toLowerANSI (configName _x), true, true];
			};
		};
	} forEach ("true" configClasses (missionConfigFile >> "CfgRemoteExec" >> "Commands"));
};

if (KH_var_remoteExecFunctionsMode isEqualTo 1) then {
	{
		if (isNumber (_x >> "allowedTargets")) then {
			if ((getNumber (_x >> "allowedTargets")) isNotEqualTo 0) then {
				KH_var_remoteExecCommandsBlacklist set [toLowerANSI (configName _x), true];
			};
		};

		if (isNumber (_x >> "jip")) then {
			if ((getNumber (_x >> "jip")) isEqualTo 0) then {
				KH_var_remoteExecCommandsJipBlacklist set [toLowerANSI (configName _x), true];
			};
		};
	} forEach ("true" configClasses (configFile >> "CfgRemoteExec" >> "Functions"));

	{
		if (isNumber (_x >> "allowedTargets")) then {
			if ((getNumber (_x >> "allowedTargets")) isNotEqualTo 0) then {
				KH_var_remoteExecFunctionsBlacklist set [toLowerANSI (configName _x), true, true];
			};
		};

		if (isNumber (_x >> "jip")) then {
			if ((getNumber (_x >> "jip")) isEqualTo 0) then {
				KH_var_remoteExecFunctionsJipBlacklist set [toLowerANSI (configName _x), true, true];
			};
		};
	} forEach ("true" configClasses (missionConfigFile >> "CfgRemoteExec" >> "Functions"));
};

[
	"MISSION",
	"EachFrame",
	[], 
	{
		if (KH_var_entityInitializationsDeletions isNotEqualTo []) then {
			private _entityInitializationsDeletions = [];

			{
				if ((_x select 3) in KH_var_entityInitializationsDeletions) then {
					_entityInitializationsDeletions pushBack _forEachIndex;
				};
			} forEach KH_var_entityInitializations;

			KH_var_entityInitializations deleteAt _entityInitializationsDeletions;
			KH_var_entityInitializationsDeletions resize 0;
		};

		if (KH_var_temporalExecutionStackAdditions isNotEqualTo []) then {
			KH_var_temporalExecutionStack append KH_var_temporalExecutionStackAdditions;
			KH_var_temporalExecutionStackAdditions resize 0;
		};

		if (KH_var_temporalExecutionStackDeletions isNotEqualTo []) then {
			{
				if ((_x select 6) in KH_var_temporalExecutionStackDeletions) then {
					KH_var_temporalExecutionStackDeletions set [KH_var_temporalExecutionStackDeletions find (_x select 6), _forEachIndex];
				};
			} forEach KH_var_temporalExecutionStack;

			KH_var_temporalExecutionStack deleteAt (KH_var_temporalExecutionStackDeletions select {_x isEqualType 0;});
			KH_var_temporalExecutionStackDeletions resize 0;
		};

		{
			_x params ["_args", "_function", "_delay", "_delta", "_totalDelta", "_handlerId", "_eventName", "_previousReturn", "_executionTime", "_executionCount"];

			if (_eventName in KH_var_temporalExecutionStackDeletions) then {
				continue;
			};

			if (_delay > 0) then {
				if (diag_tickTime >= _delta) then {
					_totalDelta = if (_totalDelta isEqualTo -1) then {
						diag_deltaTime;
					}
					else {
						_x set [4, getEpoch];
						getEpochDelta _totalDelta;
					};

					_x set [7, _args call _function];
					_x set [3, _delta + _delay];
					_x set [9, _executionCount + 1];
				};
			}
			else {
				if (diag_frameNo >= _delta) then {
					_totalDelta = if (_totalDelta isEqualTo -1) then {
						diag_deltaTime;
					}
					else {
						_x set [4, getEpoch];
						getEpochDelta _totalDelta;
					};

					_x set [7, _args call _function];
					_x set [3, _delta + (abs _delay)];
					_x set [9, _executionCount + 1];
				};
			};
		} forEach KH_var_temporalExecutionStack;
	}
] call KH_fnc_addEventHandler;

[
	"CBA",
	"KH_eve_temporalExecutionStackHandler",
	[], 
	{
		params ["_handlerId", "_deleteHandler", "_overrideTimeoutOnDeletion", "_conditionFailure"];
		private _currentHandler = KH_var_temporalExecutionStackMonitor get _handlerId;
		if (isNil "_currentHandler") exitWith {};
		_currentHandler params ["_timeoutArguments", "_timeoutFunction", "_handlerTickCounter", "_timeout", "_timeoutOnDeletion"];

		if _deleteHandler exitWith {
			if !(_handlerId in KH_var_temporalExecutionStackDeletions) then {
				KH_var_temporalExecutionStackDeletions pushBackUnique _handlerId;

				if (_timeoutOnDeletion || _overrideTimeoutOnDeletion) then {
					_timeoutArguments call _timeoutFunction;
				};
			};

			missionNamespace setVariable [_handlerTickCounter, nil];
			KH_var_temporalExecutionStackMonitor deleteAt _handlerId;
		};

		if ((missionNamespace getVariable [_handlerTickCounter, 1]) >= _timeout) then {
			if !(_handlerId in KH_var_temporalExecutionStackDeletions) then {
				KH_var_temporalExecutionStackDeletions pushBackUnique _handlerId;
				_timeoutArguments call _timeoutFunction;
			};

			missionNamespace setVariable [_handlerTickCounter, nil];
			KH_var_temporalExecutionStackMonitor deleteAt _handlerId;
		}
		else {
			missionNamespace setVariable [_handlerTickCounter, (missionNamespace getVariable [_handlerTickCounter, 1]) + 1];
		};
	}
] call KH_fnc_addEventHandler;

[
	"CBA",
	"KH_eve_registerCallback",
	[], 
	{
		params ["_arguments", ["_function", "", [""]], ["_caller", 2, [0]], ["_unscheduled", true, [true]], ["_callbackId", "", [""]]];
		[_callbackId, [_arguments, _function, _caller, _unscheduled] call KH_fnc_callSerializedFunction, _caller, false] call KH_fnc_triggerCbaEvent;		
	}
] call KH_fnc_addEventHandler;

[
	"MISSION",
	"EntityCreated",
	[], 
	{
		params ["_entity"];

		[
			[_entity],
			{
				params ["_entity"];

				{
					_x params ["_typeInclude", "_typeExclude", "_function", "_id"];

					if !(missionNamespace getVariable _id) then {
						KH_var_entityInitializationsDeletions pushBack _id;
						continue;
					};

					private _continue = true;

					{
						if (_entity isKindOf _x) then {
							_continue = false;
							break;
						};
					} forEach _typeExclude;
					
					if !_continue then {
						continue;
					};

					_continue = false;

					{
						if (_entity isKindOf _x) then {
							_continue = true;
							break;
						};
					} forEach _typeInclude;

					if !_continue then {
						continue;
					};

					[_entity] call _function;
				} forEach KH_var_entityInitializations;
			},
			true,
			"-2",
			false
		] call KH_fnc_execute;
	}
] call KH_fnc_addEventHandler;

[
	"CBA",
	"KH_eve_meleeInternalGotHit",
	[],
	{
		params ["_unit", "_instigator", "_selection", "_position", "_attack"];
		private _continue = true;

		if ((_unit getVariable ["KH_var_currentMeleeDodgeDirection", -1]) isNotEqualTo -1) then {
			private _difference = (abs ((_unit getVariable ["KH_var_currentMeleeDodgeDirection", -1]) - (_unit getRelDir _instigator))) mod 360;

			if (_difference > 180) then {
				_difference = 360 - _difference;
			};
			
			if (_difference >= KH_var_meleeDodgeFailureAngleRange) then {
				_continue = false;
			};
		};
		
		if !_continue exitWith {};
		private _unitConfig = configFile >> "CfgKHMeleeTypes" >> (_unit getVariable ["KH_var_meleeType", ""]);
		private _instigatorConfig = configFile >> "CfgKHMeleeTypes" >> (_instigator getVariable ["KH_var_meleeType", ""]);
		private _hitBlockPower = (getNumber (_instigatorConfig >> _attack >> "blockPower")) * (1 - ((getFatigue _instigator) * KH_var_meleePowerReductionStaminaCoefficient));
		private _blockPower = (getNumber (_unitConfig >> (_unit getVariable ["KH_var_currentMeleeBlock", ""]) >> "power")) * (1 - ((getFatigue _unit) * KH_var_meleePowerReductionStaminaCoefficient));
		private _hitParryPower = (getNumber (_instigatorConfig >> _attack >> "parryPower")) * (1 - ((getFatigue _instigator) * KH_var_meleePowerReductionStaminaCoefficient));
		private _parryPower = (getNumber (_unitConfig >> (_unit getVariable ["KH_var_currentMeleeParry", ""]) >> "power")) * (1 - ((getFatigue _unit) * KH_var_meleePowerReductionStaminaCoefficient));
		private _surfaceBlockPower = getNumber ((configOf _unit) >> "kh_meleeSurfaceBlockPower");
		_blockPower = _blockPower + _surfaceBlockPower;
		private _isBlocking = (_instigator in (_unit getVariable ["KH_var_meleeBlockedUnits", []])) || (_surfaceBlockPower >= _hitBlockPower);
		private _blocked = _isBlocking && (_blockPower >= _hitBlockPower);
		private _parried = (_instigator in (_unit getVariable ["KH_var_meleeParriedUnits", []])) && (_parryPower >= _hitParryPower);

		if _isBlocking then {
			[
				[_unit, getNumber (_unitConfig >> (_unit getVariable ["KH_var_currentMeleeBlock", ""]) >> "cost")],
				{
					params ["_unit", "_cost"];

					_unit setFatigue (
						(
							(getFatigue _unit) + 
							(
								_cost * 
								KH_var_meleeAbsoluteBlockStaminaConsumptionMultiplier * 
								KH_var_meleeAbsoluteStaminaConsumptionMultiplier
							)
						) min 1
					);
				},
				_unit,
				true,
				false
			] call KH_fnc_execute;
		};

		[
			"KH_eve_meleeHasHit", 
			[_instigator, _unit, _attack, _hitBlockPower, _hitParryPower, _position, _blockPower, _parryPower, _blocked, _parried], 
			_instigator, 
			false
		] call KH_fnc_triggerCbaEvent;

		[
			"KH_eve_meleeGotHit", 
			[_unit, _instigator, _attack, _hitBlockPower, _hitParryPower, _position, _blockPower, _parryPower, _blocked, _parried],
			_unit,
			false
		] call KH_fnc_triggerCbaEvent;

		if (!_blocked && !_parried) then {
			private _damageFunction = missionNamespace getVariable [getText (_instigatorConfig >> _attack >> "damageFunction"), {}];

			private _playSound = if (_damageFunction isNotEqualTo {}) then {
				if ([_unit, _instigator, ["HIT", _attack, _hitBlockPower, _hitParryPower, _position, _blockPower, _parryPower]] call _damageFunction) then {
					[[_unit, ["HIT", [getNumber (_instigatorConfig >> _attack >> "kickPower"), getNumber (_instigatorConfig >> _attack >> "tacklePower")], _unit getRelDir _instigator]], "KH_fnc_updateMeleeState", _unit, true, false] call KH_fnc_execute;
					[[_unit, _selection, getText (_instigatorConfig >> _attack >> "type"), _instigator], "KH_fnc_simulateHit", "SERVER", true, false] call KH_fnc_execute;
					true;
				}
				else {
					false;
				};
			}
			else {
				[[_unit, ["HIT", [getNumber (_instigatorConfig >> _attack >> "kickPower"), getNumber (_instigatorConfig >> _attack >> "tacklePower")], _unit getRelDir _instigator]], "KH_fnc_updateMeleeState", _unit, true, false] call KH_fnc_execute;
				[[_unit, _selection, getText (_instigatorConfig >> _attack >> "type"), _instigator], "KH_fnc_simulateHit", "SERVER", true, false] call KH_fnc_execute;
				true;
			};

			if _playSound then {
				private _sounds = getArray (_instigatorConfig >> _attack >> "Sounds" >> (getText ((configOf _unit) >> "kh_meleeSoundType")));

				if (_sounds isNotEqualTo []) then {
					playSound3D [((getArray (configFile >> "CfgSounds" >> (selectRandom _sounds) >> "sound")) select 0) select [1], _unit, (insideBuilding _unit) >= 0.5, getPosASL _unit, 5, 1, 100, 0, false];
				}
				else {
					_sounds = getArray (_instigatorConfig >> _attack >> "Sounds" >> "generic");

					if (_sounds isNotEqualTo []) then {
						playSound3D [((getArray (configFile >> "CfgSounds" >> (selectRandom _sounds) >> "sound")) select 0) select [1], _unit, (insideBuilding _unit) >= 0.5, getPosASL _unit, 5, 1, 100, 0, false];
					};
				};
			};

			if (KH_var_meleeAttackBlockOnInsufficientStamina && ((getFatigue _instigator) >= (1 - (getNumber (_instigatorConfig >> _attack >> "cost"))))) then {
				[[_instigator, "BLOCKED"], "KH_fnc_updateMeleeState", _instigator, true, false] call KH_fnc_execute;
			};

			[
				[
					_unit,
					getNumber (_instigatorConfig >> _attack >> "costInfliction")
				],
				{
					params ["_unit", "_costInfliction"];
					
					_unit setFatigue (
						(
							(getFatigue _unit) + 
							(_costInfliction * KH_var_meleeAbsoluteAttackStaminaExhaustionMultiplier * KH_var_meleeAbsoluteStaminaExhaustionMultiplier)
						) min 1
					);
				},
				_unit,
				true,
				false
			] call KH_fnc_execute;
		}
		else {
			if _blocked then {
				[[_unit, "BLOCK_SUCCESS"], "KH_fnc_updateMeleeState", _unit, true, false] call KH_fnc_execute;
			};

			[
				[
					_instigator,
					["BLOCKED", "PARRIED"] select _parried,
					if _parried then {
						getNumber (_unitConfig >> (_unit getVariable ["KH_var_currentMeleeParry", ""]) >> "costInfliction");
					}
					else {
						getNumber (_unitConfig >> (_unit getVariable ["KH_var_currentMeleeBlock", ""]) >> "costInfliction");
					},
					_parried
				],
				{
					params ["_instigator", "_action", "_costInfliction", "_parried"];
					
					_instigator setFatigue (
						(
							(getFatigue _instigator) + 
							(_costInfliction * ([KH_var_meleeAbsoluteBlockStaminaExhaustionMultiplier, KH_var_meleeAbsoluteParryStaminaExhaustionMultiplier] select _parried) * KH_var_meleeAbsoluteStaminaExhaustionMultiplier)
						) min 1
					);

					[_instigator, _action] call KH_fnc_updateMeleeState;
				},
				_instigator,
				true,
				false
			] call KH_fnc_execute;

			private _sounds = getArray (_instigatorConfig >> _attack >> "Sounds" >> (["parried", "blocked"] select _blocked));

			if (_sounds isNotEqualTo []) then {
				playSound3D [((getArray (configFile >> "CfgSounds" >> (selectRandom _sounds) >> "sound")) select 0) select [1], _unit, (insideBuilding _unit) >= 0.5, getPosASL _unit, 5, 1, 100, 0, false];
			}
			else {
				_sounds = getArray (_instigatorConfig >> _attack >> "Sounds" >> (["blocked", "parried"] select _parried));

				if (_sounds isNotEqualTo []) then {
					playSound3D [((getArray (configFile >> "CfgSounds" >> (selectRandom _sounds) >> "sound")) select 0) select [1], _unit, (insideBuilding _unit) >= 0.5, getPosASL _unit, 5, 1, 100, 0, false];
				};
			};
		};
	}
] call KH_fnc_addEventHandler;

[
	"CBA",
	"KH_eve_meleeInternalGotKicked",
	[],
	{
		params ["_unit", "_instigator", "_selection", "_position", "_kick"];
		private _continue = true;

		if ((_unit getVariable ["KH_var_currentMeleeDodgeDirection", -1]) isNotEqualTo -1) then {
			private _difference = (abs ((_unit getVariable ["KH_var_currentMeleeDodgeDirection", -1]) - (_unit getRelDir _instigator))) mod 360;

			if (_difference > 180) then {
				_difference = 360 - _difference;
			};
			
			if (_difference >= KH_var_meleeDodgeFailureAngleRange) then {
				_continue = false;
			};
		};
		
		if !_continue exitWith {};
		private _unitConfig = configFile >> "CfgKHMeleeTypes" >> (_unit getVariable ["KH_var_meleeType", ""]);
		private _instigatorConfig = configFile >> "CfgKHMeleeTypes" >> (_instigator getVariable ["KH_var_meleeType", ""]);
		private _kickPower = (getNumber (_instigatorConfig >> _kick >> "power")) * (1 - ((getFatigue _instigator) * KH_var_meleePowerReductionStaminaCoefficient));
		private _blockPower = (getNumber (_unitConfig >> (_unit getVariable ["KH_var_currentMeleeBlock", ""]) >> "power")) * (1 - ((getFatigue _unit) * KH_var_meleePowerReductionStaminaCoefficient));
		private _isBlocking = _instigator in (_unit getVariable ["KH_var_meleeBlockedUnits", []]);
		private _blocked = _isBlocking && (_blockPower >= _kickPower);

		if _isBlocking then {
			[
				[_unit, getNumber (_unitConfig >> (_unit getVariable ["KH_var_currentMeleeBlock", ""]) >> "cost")],
				{
					params ["_unit", "_cost"];

					_unit setFatigue (
						(
							(getFatigue _unit) + 
							(
								_cost * 
								KH_var_meleeAbsoluteBlockStaminaConsumptionMultiplier * 
								KH_var_meleeAbsoluteStaminaConsumptionMultiplier
							)
						) min 1
					);
				},
				_unit,
				true,
				false
			] call KH_fnc_execute;
		};

		[
			"KH_eve_meleeHasKicked", 
			[_instigator, _unit, _kick, _position, _kickPower, _blockPower, _blocked], 
			_instigator, 
			false
		] call KH_fnc_triggerCbaEvent;

		[
			"KH_eve_meleeGotKicked", 
			[_unit, _instigator, _kick, _position, _kickPower, _blockPower, _blocked],
			_unit,
			false
		] call KH_fnc_triggerCbaEvent;

		if !_blocked then {
			private _damageFunction = missionNamespace getVariable [getText (_instigatorConfig >> _kick >> "damageFunction"), {}];

			if (_damageFunction isNotEqualTo {}) then {
				if ([_unit, _instigator, ["KICK", _kick, _position, _kickPower, _blockPower]] call _damageFunction) then {
					[[_unit, ["STAGGER", false, _unit getRelDir _instigator]], "KH_fnc_updateMeleeState", _unit, true, false] call KH_fnc_execute;
					[[_unit, _selection, getText (_instigatorConfig >> _kick >> "type"), _instigator], "KH_fnc_simulateHit", "SERVER", true, false] call KH_fnc_execute;
				};
			}
			else {
				[[_unit, ["STAGGER", false, _unit getRelDir _instigator]], "KH_fnc_updateMeleeState", _unit, true, false] call KH_fnc_execute;
				[[_unit, _selection, getText (_instigatorConfig >> _kick >> "type"), _instigator], "KH_fnc_simulateHit", "SERVER", true, false] call KH_fnc_execute;
			};

			[
				[
					_unit,
					getNumber (_instigatorConfig >> _kick >> "costInfliction")
				],
				{
					params ["_unit", "_costInfliction"];
					
					_unit setFatigue (
						(
							(getFatigue _unit) + 
							(_costInfliction * KH_var_meleeAbsoluteKickStaminaExhaustionMultiplier * KH_var_meleeAbsoluteStaminaExhaustionMultiplier)
						) min 1
					);
				},
				_unit,
				true,
				false
			] call KH_fnc_execute;
		}
		else {
			[[_unit, "BLOCK_SUCCESS"], "KH_fnc_updateMeleeState", _unit, true, false] call KH_fnc_execute;
		};

		private _sounds = getArray (_instigatorConfig >> _kick >> "Sounds" >> (getText ((configOf _unit) >> "kh_meleeSoundType")));

		if (_sounds isNotEqualTo []) then {
			playSound3D [((getArray (configFile >> "CfgSounds" >> (selectRandom _sounds) >> "sound")) select 0) select [1], _unit, (insideBuilding _unit) >= 0.5, getPosASL _unit, 5, 1, 100, 0, false];
		}
		else {
			_sounds = getArray (_instigatorConfig >> _kick >> "Sounds" >> "generic");

			if (_sounds isNotEqualTo []) then {
				playSound3D [((getArray (configFile >> "CfgSounds" >> (selectRandom _sounds) >> "sound")) select 0) select [1], _unit, (insideBuilding _unit) >= 0.5, getPosASL _unit, 5, 1, 100, 0, false];
			};
		};
	}
] call KH_fnc_addEventHandler;

[
	"CBA",
	"KH_eve_meleeInternalGotTackled",
	[],
	{
		params ["_unit", "_instigator", "_tackle"];
		private _continue = true;

		if ((_unit getVariable ["KH_var_currentMeleeDodgeDirection", -1]) isNotEqualTo -1) then {
			private _difference = (abs ((_unit getVariable ["KH_var_currentMeleeDodgeDirection", -1]) - (_unit getDir _instigator))) mod 360;

			if (_difference > 180) then {
				_difference = 360 - _difference;
			};
			
			if (_difference >= KH_var_meleeDodgeFailureAngleRange) then {
				_continue = false;
			};
		};
		
		if !_continue exitWith {};
		private _unitConfig = configFile >> "CfgKHMeleeTypes" >> (_unit getVariable ["KH_var_meleeType", ""]);
		private _instigatorConfig = configFile >> "CfgKHMeleeTypes" >> (_instigator getVariable ["KH_var_meleeType", ""]);
		private _tacklePower = (getNumber (_instigatorConfig >> _tackle >> "power")) * (1 - ((getFatigue _instigator) * KH_var_meleePowerReductionStaminaCoefficient));
		private _blockPower = (getNumber (_unitConfig >> (_unit getVariable ["KH_var_currentMeleeBlock", ""]) >> "power")) * (1 - ((getFatigue _unit) * KH_var_meleePowerReductionStaminaCoefficient));
		private _isBlocking = _instigator in (_unit getVariable ["KH_var_meleeBlockedUnits", []]);
		private _blocked = _isBlocking && (_blockPower >= _tacklePower);

		if _isBlocking then {
			[
				[_unit, ((getNumber (_unitConfig >> (_unit getVariable ["KH_var_currentMeleeBlock", ""]) >> "cost")) + (getNumber (_instigatorConfig >> _tackle >> "costBlockInfliction"))) min 1],
				{
					params ["_unit", "_cost"];

					_unit setFatigue (
						(
							(getFatigue _unit) + 
							(
								_cost * 
								KH_var_meleeAbsoluteBlockStaminaConsumptionMultiplier *
								KH_var_meleeAbsoluteTackleBlockStaminaExhaustionMultiplier *  
								KH_var_meleeAbsoluteStaminaConsumptionMultiplier
							)
						) min 1
					);
				},
				_unit,
				true,
				false
			] call KH_fnc_execute;
		};

		[
			"KH_eve_meleeHasTackled", 
			[_instigator, _unit, _tackle, _tacklePower, _blockPower, _blocked], 
			_instigator, 
			false
		] call KH_fnc_triggerCbaEvent;

		[
			"KH_eve_meleeGotTackled", 
			[_unit, _instigator, _tackle, _tacklePower, _blockPower, _blocked],
			_unit,
			false
		] call KH_fnc_triggerCbaEvent;

		if !_blocked then {
			private _damageFunction = missionNamespace getVariable [getText (_instigatorConfig >> _tackle >> "damageFunction"), {}];

			if (_damageFunction isNotEqualTo {}) then {
				if ([_unit, _instigator, ["TACKLE", _tackle, _tacklePower, _blockPower]] call _damageFunction) then {
					[[_unit, ["STAGGER", true, _unit getRelDir _instigator]], "KH_fnc_updateMeleeState", _unit, true, false] call KH_fnc_execute;
					[[_unit, selectRandom ((_unit selectionNames "FireGeometry") select {"hit" in _x;}), getText (_instigatorConfig >> _tackle >> "type"), _instigator], "KH_fnc_simulateHit", "SERVER", true, false] call KH_fnc_execute;
				};
			}
			else {
				[[_unit, ["STAGGER", true, _unit getRelDir _instigator]], "KH_fnc_updateMeleeState", _unit, true, false] call KH_fnc_execute;
				[[_unit, selectRandom ((_unit selectionNames "FireGeometry") select {"hit" in _x;}), getText (_instigatorConfig >> _tackle >> "type"), _instigator], "KH_fnc_simulateHit", "SERVER", true, false] call KH_fnc_execute;
			};

			[
				[
					_unit,
					getNumber (_instigatorConfig >> _tackle >> "costInfliction")
				],
				{
					params ["_unit", "_costInfliction"];
					
					_unit setFatigue (
						(
							(getFatigue _unit) + 
							(_costInfliction * KH_var_meleeAbsoluteTackleStaminaExhaustionMultiplier * KH_var_meleeAbsoluteStaminaExhaustionMultiplier)
						) min 1
					);
				},
				_unit,
				true,
				false
			] call KH_fnc_execute;
		}
		else {
			[[_unit, "BLOCK_SUCCESS"], "KH_fnc_updateMeleeState", _unit, true, false] call KH_fnc_execute;
		};

		private _sounds = getArray (_instigatorConfig >> _tackle >> "Sounds" >> (getText ((configOf _unit) >> "kh_meleeSoundType")));

		if (_sounds isNotEqualTo []) then {
			playSound3D [((getArray (configFile >> "CfgSounds" >> (selectRandom _sounds) >> "sound")) select 0) select [1], _unit, (insideBuilding _unit) >= 0.5, getPosASL _unit, 5, 1, 100, 0, false];
		}
		else {
			_sounds = getArray (_instigatorConfig >> _tackle >> "Sounds" >> "generic");

			if (_sounds isNotEqualTo []) then {
				playSound3D [((getArray (configFile >> "CfgSounds" >> (selectRandom _sounds) >> "sound")) select 0) select [1], _unit, (insideBuilding _unit) >= 0.5, getPosASL _unit, 5, 1, 100, 0, false];
			};
		};
	}
] call KH_fnc_addEventHandler;

if isServer then {
	KH_var_serverGameSessionId = KH_var_gameSessionId;
	publicVariable "KH_var_serverGameSessionId";
	KH_var_serverMissionSessionId = KH_var_missionSessionId;
	publicVariable "KH_var_serverMissionSessionId";
	uiNamespace setVariable ["KH_var_serverMissionSessionId", KH_var_serverMissionSessionId];
	KH_var_clientRegistered = true;
	KH_var_missionStarted = false;
	publicVariable "KH_var_missionStarted";
	KH_var_playersLoaded = false;
	publicVariable "KH_var_playersLoaded";
	KH_var_disconnectedPlayerUids = [];
	publicVariable "KH_var_disconnectedPlayerUids";
	KH_var_adminMachine = clientOwner;
	publicVariable "KH_var_adminMachine";
	KH_var_allMachines = [clientOwner];
	publicVariable "KH_var_allMachines";
	KH_var_allIdMachines = createHashMap;
	publicVariable "KH_var_allIdMachines";
	KH_var_allCuratorMachines = [];
	publicVariable "KH_var_allCuratorMachines";
	KH_var_allHeadlessMachines = [];
	publicVariable "KH_var_allHeadlessMachines";
	KH_var_allHeadlessIdMachines = createHashMap;
	publicVariable "KH_var_allHeadlessIdMachines";
	KH_var_allHeadlessUnits = [];
	publicVariable "KH_var_allHeadlessUnits";
	KH_var_allPlayerMachines = [];
	publicVariable "KH_var_allPlayerMachines";
	KH_var_allPlayerIdMachines = createHashMap;
	publicVariable "KH_var_allPlayerIdMachines";
	KH_var_allPlayerUidMachines = createHashMap;
	publicVariable "KH_var_allPlayerUidMachines";
	KH_var_allPlayerUnits = [];
	publicVariable "KH_var_allPlayerUnits";
	KH_var_initialPlayerUnits = [];
	publicVariable "KH_var_initialPlayerUnits";
	KH_var_initialPlayerMachines = [];
	publicVariable "KH_var_initialPlayerMachines";
	KH_var_jipPlayerUnits = [];
	publicVariable "KH_var_jipPlayerUnits";
	KH_var_jipPlayerMachines = [];
	publicVariable "KH_var_jipPlayerMachines";
	KH_var_allPlayerControlledUnits = [];
	publicVariable "KH_var_allPlayerControlledUnits";
	KH_var_allPlayerControlledUnitsMachines = createHashMap;
	publicVariable "KH_var_allPlayerControlledUnitsMachines";
	KH_var_logicGroup = createGroup [sideLogic, false];
	publicVariable "KH_var_logicGroup";
	KH_var_helperLogic = KH_var_logicGroup createUnit ["Logic", [0, 0, 0], [], 0, "CAN_COLLIDE"];
	publicVariable "KH_var_helperLogic";
	KH_var_helperObject = createVehicle ["KH_HelperSquare", [0, 0, 0], [], 0, "CAN_COLLIDE"];
	publicVariable "KH_var_helperObject";
	KH_var_simpleHelperObject = createSimpleObject ["KH_HelperSquare", [0, 0, 0], false];
	publicVariable "KH_var_simpleHelperObject";
	KH_var_diagnosticsState = false;
	publicVariable "KH_var_diagnosticsState";
	KH_var_missionSuspended = false;
	publicVariable "KH_var_missionSuspended";
	KH_var_jipHandlers = createHashMap;
	KH_var_headlessClientTransfers = [];
	KH_var_entityArrayBuilderArrays = [];
	KH_var_groupArrayBuilderArrays = [];
	KH_var_initialSideRelations = [];
	KH_fnc_serverMissionLoadInit = {};
	KH_fnc_serverMissionStartInit = {};
	KH_fnc_serverPlayersLoadedInit = {};
	KH_fnc_serverMissionEndInit = {};
	KH_var_serverMissionLoadStack = [];
	KH_var_serverMissionStartStack = [];
	KH_var_serverPlayersLoadedStack = [];
	KH_var_serverMissionEndStack = [];

	{
		private _basePath = (getText (_x >> "path")) regexReplace ["(/)", "\\"];

		{
			private _config = _x;

			{
				if (isNumber (_config >> _x)) then {
					if ((getNumber (_config >> _x)) isEqualTo 1) then {
						(missionNamespace getVariable (["kh_var_", _x, "stack"] joinString "")) pushBack (compile (preprocessFileLineNumbers ([
							_basePath,
							["", "\"] select (_basePath isNotEqualTo ""),
							if (isText (_config >> "path")) then {
								(getText (_config >> "path")) regexReplace ["(/)", "\\"];
							}
							else {
								"";
							}, 
							configName _config, 
							".sqf"
						] joinString "")));
					};
				};
			} forEach ["servermissionload", "servermissionstart", "serverplayersloaded", "servermissionend"];
		} forEach ("true" configClasses _x);
	} forEach (("true" configClasses (configFile >> "CfgKhInitFunctions")) + ("true" configClasses (missionConfigFile >> "CfgKhInitFunctions")));

	[
		[],
		{
			KH_var_networkingSettings = [
				KH_var_networkingPort,
				KH_var_networkingMaximumMessageSize,
				KH_var_networkingReceiveBufferSize,
				KH_var_networkingSendBufferSize,
				KH_var_networkingConnectionTimeout,
				KH_var_networkingSendTimeout,
				KH_var_networkingReceiveTimeout,
				KH_var_networkingClientStallTimeout,
				KH_var_networkingKeepAliveTime,
				KH_var_networkingKeepAliveInterval,
				KH_var_networkingSendBatchSize,
				KH_var_networkingCompression,
				KH_var_networkingCoalesceMessages,
				KH_var_networkingMaximumCoalesceSize,
				KH_var_networkingMaximumCoalescedMessages,
				KH_var_networkingCoalesceDelay
			];

			khNetworkInitialize;
		},
		true,
		{
			(
				!(isNil "KH_var_networkingPort") &&
				!(isNil "KH_var_networkingMaximumMessageSize") &&
				!(isNil "KH_var_networkingReceiveBufferSize") &&
				!(isNil "KH_var_networkingSendBufferSize") &&
				!(isNil "KH_var_networkingConnectionTimeout") &&
				!(isNil "KH_var_networkingSendTimeout") &&
				!(isNil "KH_var_networkingReceiveTimeout") &&
				!(isNil "KH_var_networkingClientStallTimeout") &&
				!(isNil "KH_var_networkingKeepAliveTime") &&
				!(isNil "KH_var_networkingKeepAliveInterval") &&
				!(isNil "KH_var_networkingSendBatchSize") &&
				!(isNil "KH_var_networkingCompression") &&
				!(isNil "KH_var_networkingCoalesceMessages") &&
				!(isNil "KH_var_networkingMaximumCoalesceSize") &&
				!(isNil "KH_var_networkingMaximumCoalescedMessages") &&
				!(isNil "KH_var_networkingCoalesceDelay")
			);
		},
		false
	] call KH_fnc_execute;

	{
		if ((_x getUserInfo 8) isNotEqualTo 0) then {
			KH_var_adminMachine = _x getUserInfo 1;
			publicVariable "KH_var_adminMachine";
			break;
		};
	} forEach allUsers;

	[
		"CBA",
		"KH_eve_jipSetup",
		[],
		{
			params ["_name", "_arguments", "_dependency", "_unitRequired", "_jipId"];
			private _currentHandler = KH_var_jipHandlers get _jipId;
			private _continue = true;

			if !(isNil "_currentHandler") then {
				if ((_currentHandler select 4) isEqualTo _unitRequired) then {
					KH_var_jipHandlers set [_jipId, [["JIP_HANDLER", _jipId], _name, _arguments, _dependency, _unitRequired, []]];
					_continue = false;
				}
				else {
					{
						[_x] call KH_fnc_removeHandler;
					} forEach (_currentHandler select 5);
				};
			};

			if !_continue exitWith {};
			private _jipHandlers = [];
			KH_var_jipHandlers set [_jipId, [["JIP_HANDLER", _jipId], _name, _arguments, _dependency, _unitRequired, _jipHandlers]];
			
			private _joinType = if _unitRequired then {
				["KH_eve_playerLoaded", "KH_eve_headlessLoaded"];
			}
			else {
				["KH_eve_playerMissionLoaded", "KH_eve_headlessMissionLoaded"];
			};

			{
				_jipHandlers pushBack ([
					"CBA",
					_x,
					[_jipId, _x],
					{
						params ["_joiningMachine", "_id"];
						_args params ["_jipId", "_joinType"];

						if !(missionNamespace getVariable _jipId) exitWith {
							KH_var_jipHandlers deleteAt _jipId;
							[_handlerId] call KH_fnc_removeHandler;
						};

						private _currentHandler = KH_var_jipHandlers get _jipId;
						if (isNil "_currentHandler") exitWith {};
						private _dependency = _currentHandler select 3;
						private _condition = true;

						switch (typeName _dependency) do {
							case "BOOL": {
								if !_dependency then {
									_condition = false;
								};
							};

							case "SCALAR": {
								if !(_dependency in KH_var_allMachines) then {
									_condition = false;
								};
							};

							case "OBJECT": {
								if (isNull _dependency) then {
									_condition = false;
								};
							};

							case "TEAM_MEMBER": {
								if (isNull _dependency) then {
									_condition = false;
								};
							};

							case "GROUP": {
								if (isNull _dependency) then {
									_condition = false;
								};
							};

							case "STRING": {
								if ((parseNumber (_dependency select [0, 1])) isNotEqualTo 0) then {
									if (_dependency isNotEqualTo _id) then {
										if (":" in _dependency) then {
											if ((isNull (objectFromNetId _dependency)) && (isNull (groupFromNetId _dependency))) then {
												_condition = false;
											};
										}
										else {
											if (isNil {KH_var_allIdMachines get _x;}) then {
												_condition = false;
											};
										};
									};
								}
								else {
									if !(missionNamespace getVariable [_dependency, false]) then {
										_condition = false;
									};
								};
							};

							case "ARRAY": {																					
								{
									switch (typeName _x) do {
										case "SCALAR": {
											if !(_x in KH_var_allMachines) then {
												_condition = false;
												break;
											};
										};

										case "OBJECT": {
											if (isNull _x) then {
												_condition = false;
												break;
											};
										};

										case "TEAM_MEMBER": {
											if (isNull _x) then {
												_condition = false;
												break;
											};
										};

										case "GROUP": {
											if (isNull _x) then {
												_condition = false;
												break;
											};
										};

										case "STRING": {
											if ((parseNumber (_x select [0, 1])) isNotEqualTo 0) then {
												if (_x isNotEqualTo _id) then {
													if (":" in _x) then {
														if ((isNull (objectFromNetId _x)) && (isNull (groupFromNetId _x))) then {
															_condition = false;
															break;
														};
													}
													else {
														if (isNil {KH_var_allIdMachines get _x;}) then {
															_condition = false;
															break;
														};
													};
												};
											}
											else {
												if !(missionNamespace getVariable [_x, false]) then {
													_condition = false;
													break;
												};
											};
										};

										case "CODE": {
											if !(call _x) then {
												_condition = false;
												break;
											};
										};
									};
								} forEach _dependency;
							};

							case "CODE": {
								if !(call _dependency) then {
									_condition = false;
								};
							};
						};

						if _condition then {
							[_currentHandler select 1, _currentHandler select 2, _joiningMachine, false] call KH_fnc_triggerCbaEvent;
						};
					}
				] call KH_fnc_addEventHandler);
			} forEach _joinType;
		}
	] call KH_fnc_addEventHandler;

	[
		"CBA",
		"KH_eve_persistentExecutionSetup",
		[],
		{
			params ["_arguments", "_function", "_target", "_sendoffArguments", "_sendoffFunction", "_caller", "_unscheduled", "_persistentExecutionId"];
			private _persistentEventId = ["KH_var_persistentEventId", _persistentExecutionId] joinString "_";

			if (_target isNil _persistentEventId) then {
				_target setVariable [_persistentEventId, [_arguments, _function, _sendoffArguments, _sendoffFunction, _caller, _unscheduled], true];
				
				[
					"KH_eve_execution",
					[
						[
							["ENTITY", _target, "LOCAL"],
							"Local",
							[_persistentExecutionId, _persistentEventId],
							{
								params ["_entity", "_local"];
								_args params ["_persistentExecutionId", "_persistentEventId"];

								if (_entity getVariable _persistentExecutionId) then {
									(_entity getVariable _persistentEventId) params ["_arguments", "_function", "_sendoffArguments", "_sendoffFunction", "_caller", "_unscheduled"];
									
									if _local then {
										[_arguments, _function, _caller, _unscheduled] call KH_fnc_callSerializedFunction;
									}
									else {
										[_sendoffArguments, _sendoffFunction, _caller, _unscheduled] call KH_fnc_callSerializedFunction;
									};
								};
							}
						], 
						"KH_fnc_addEventHandler", 
						_caller
					],
					"GLOBAL",
					[_target, false, _persistentExecutionId]
				] call KH_fnc_triggerCbaEvent;
			};
		}
	] call KH_fnc_addEventHandler;

	[
		"CBA",
		"KH_eve_playerPresenceExecutionSetup",
		[],
		{
			params ["_arguments", "_function", "_caller", "_unscheduled", "_object", "_present", "_distance", "_nearId", "_units", "_jip"];

			[
				[_arguments, _function, _caller, _unscheduled, _object, _present, _distance, _nearId, _units],
				{
					params ["_arguments", "_function", "_caller", "_unscheduled", "_object", "_present", "_distance", "_nearId", "_units"];
				
					if ((isNull _object) || (_units isEqualTo []) || !(missionNamespace getVariable _nearId)) exitWith {
						[_handlerId] call KH_fnc_removeHandler;
					};

					private _deletions = [];

					{
						private _object = param [4];
						private _distance = param [6];

						if _present then {
							if ((_x distance _object) <= _distance) then {
								["KH_eve_execution", [_arguments, _function, _caller, _unscheduled], _x, false] call KH_fnc_triggerCbaEvent;
								_deletions pushBack _forEachIndex;
							};
						}
						else {
							if ((_x distance _object) > _distance) then {
								["KH_eve_execution", [_arguments, _function, _caller, _unscheduled], _x, false] call KH_fnc_triggerCbaEvent;
								_deletions pushBack _forEachIndex;
							};
						};
					} forEach _units;

					_units deleteAt _deletions;
				},
				true,
				0,
				false
			] call KH_fnc_execute;

			if _jip then {
				[
					"CBA",
					"KH_eve_playerLoaded",
					[_arguments, _function, _caller, _unscheduled, _object, _present, _distance, _nearId],
					{
						private _unit = param [3];
						_args params ["_arguments", "_function", "_caller", "_unscheduled", "_object", "_present", "_distance", "_nearId"];

						if !(missionNamespace getVariable _nearId) exitWith {
							[_handlerId] call KH_fnc_removeHandler;
						};

						[
							[_arguments, _function, _caller, _unscheduled, _object, _present, _distance, _unit],
							{
								params ["_arguments", "_function", "_caller", "_unscheduled", "_object", "_present", "_distance", "_unit"];
								_this set [7, _unit getVariable ["KH_var_playerUnit", _unit]];

								if !(missionNamespace getVariable _nearId) exitWith {
									[_handlerId] call KH_fnc_removeHandler;
								};
								
								if _present then {
									if ((_unit distance _object) <= _distance) then {
										["KH_eve_execution", [_arguments, _function, _caller, _unscheduled], _x, false] call KH_fnc_triggerCbaEvent;
										[_handlerId] call KH_fnc_removeHandler;
									};
								}
								else {
									if ((_unit distance _object) > _distance) then {
										["KH_eve_execution", [_arguments, _function, _caller, _unscheduled], _x, false] call KH_fnc_triggerCbaEvent;
										[_handlerId] call KH_fnc_removeHandler;
									};
								};
							},
							true,
							0,
							false
						] call KH_fnc_execute;
					}
				] call KH_fnc_addEventHandler;
			};
		}
	] call KH_fnc_addEventHandler;

	[
		"CBA",
		"KH_eve_playerMissionPreloaded",
		[], 
		{
			params ["_machineId"];
			KH_var_allMachines pushBackUnique _machineId;
			publicVariable "KH_var_allMachines";
			KH_var_allPlayerMachines pushBackUnique _machineId;
			publicVariable "KH_var_allPlayerMachines";
			private "_uid";
			private "_id";
			
			{
				if ((_x getUserInfo 1) isEqualTo _machineId) then {
					_uid = _x getUserInfo 2;
					_id = _x getUserInfo 0;
					KH_var_allPlayerUidMachines set [_uid, _machineId];
					publicVariable "KH_var_allPlayerUidMachines";
					KH_var_allPlayerIdMachines set [_id, _machineId];
					publicVariable "KH_var_allPlayerIdMachines";
					KH_var_allIdMachines set [_id, _machineId];
					publicVariable "KH_var_allIdMachines";
					missionNamespace setVariable ["KH_var_steamId", _uid, _machineId];
					missionNamespace setVariable ["KH_var_directPlayId", _directPlayId, _machineId];
					break;
				};
			} forEach allUsers;
			
			if KH_var_playersLoaded then {
				KH_var_jipPlayerMachines pushBackUnique _machineId;
				publicVariable "KH_var_jipPlayerMachines";
			};

			if ((admin _machineId) isNotEqualTo 0) then {
				KH_var_adminMachine = _machineId;
				publicVariable "KH_var_adminMachine";
			};

			[
				[_uid],
				{
					params ["_uid"];

					if (profileNamespace isNil "KH_var_steamId") then {
						profileNamespace setVariable ["KH_var_steamId", _uid];
					};

					KH_var_clientRegistered = true;
				},
				_machineId,
				true,
				false
			] call KH_fnc_execute;

			["KH_eve_playerMissionLoaded", [_machineId, _uid, _id], "GLOBAL", false] call KH_fnc_triggerCbaEvent;
		}
	] call KH_fnc_addEventHandler;

	[
		"CBA",
		"KH_eve_headlessMissionPreloaded",
		[], 
		{
			params ["_machineId"];
			KH_var_allMachines pushBackUnique _machineId;
			publicVariable "KH_var_allMachines";
			KH_var_allHeadlessMachines pushBackUnique _machineId;
			publicVariable "KH_var_allHeadlessMachines";
			private _id = "";

			{
				if ((_x getUserInfo 1) isEqualTo _machineId) then {
					_id = _x getUserInfo 0;
					KH_var_allIdMachines set [_id, _machineId];
					publicVariable "KH_var_allIdMachines";
					KH_var_allHeadlessIdMachines set [_id, _machineId];
					publicVariable "KH_var_allHeadlessIdMachines";
					break;
				};
			} forEach allUsers;

			missionNamespace setVariable ["KH_var_clientRegistered", true, _machineId];
			["KH_eve_headlessMissionLoaded", [_machineId, _id], "GLOBAL", false] call KH_fnc_triggerCbaEvent;
		}
	] call KH_fnc_addEventHandler;
	
	[
		"CBA",
		"KH_eve_playerLoaded",
		[],
		{
			private _unit = param [3];

			if KH_var_playersLoaded then {
				KH_var_jipPlayerUnits pushBackUnique _unit;
				publicVariable "KH_var_jipPlayerUnits";
			};

			KH_var_allPlayerUnits pushBackUnique _unit;
			publicVariable "KH_var_allPlayerUnits";
			private _arrayBuilderArray = _unit getVariable ["KH_var_assignedEntityArrayBuilderArrays", []];

			if (_arrayBuilderArray isNotEqualTo []) then {
				{
					private _entityArray = missionNamespace getVariable [_x, []];

					if !(_unit in _entityArray) then {
						_entityArray pushBackUnique _unit;
						missionNamespace setVariable [_x, _entityArray, true];
					};
				} forEach _arrayBuilderArray;
			};
		}
	] call KH_fnc_addEventHandler;

	[
		"CBA",
		"KH_eve_headlessLoaded",
		[],
		{
			private _headlessClientOwner = param [0];
			private _headlessClient = param [2];
			KH_var_allHeadlessUnits pushBackUnique _headlessClient;
			publicVariable "KH_var_allHeadlessUnits";
			private _arrayBuilderArray = _headlessClient getVariable ["KH_var_assignedEntityArrayBuilderArrays", []];

			if (_arrayBuilderArray isNotEqualTo []) then {
				{
					private _entityArray = missionNamespace getVariable [_x, []];

					if !(_headlessClient in _entityArray) then {
						_entityArray pushBackUnique _headlessClient;
						missionNamespace setVariable [_x, _entityArray, true];
					};
				} forEach _arrayBuilderArray;
			};

			private _assignedEntities = [];
			private _assignedEntitiesRecreate = [];

			{
				_x params ["_unit", "_owner", "_recreate"];
				
				if ((vehicleVarName _headlessClient) isEqualTo _owner) then {
					if !_recreate then {
						_assignedEntities pushBack _unit;
					}
					else {
						_assignedEntitiesRecreate pushBack _unit;
					};
				};
			} forEach KH_var_headlessClientTransfers;
			
			_unit setVariable ["KH_var_headlessClientTransferInit", _unit getVariable ["KH_var_headlessClientTransferInit", {}], _headlessClientOwner];

			if (_assignedEntities isNotEqualTo []) then {
				[
					_assignedEntities, 
					_headlessClientOwner, 
					false, 
					{
						params ["_unit"];
						[_unit] call (_unit getVariable "KH_var_headlessClientTransferInit");
					}
				] call KH_fnc_setOwnership;
			};

			if (_assignedEntitiesRecreate isNotEqualTo []) then {
				[
					_assignedEntitiesRecreate, 
					_headlessClientOwner, 
					true, 
					{
						params ["_unit"];
						[_unit] call (_unit getVariable "KH_var_headlessClientTransferInit");
					}
				] call KH_fnc_setOwnership;
			};
		}
	] call KH_fnc_addEventHandler;
	
	[
		"CBA",
		"KH_eve_playerSwitched",
		[], 
		{
			private _newUnit = param [3];
			private _previousUnit = param [4];

			{
				private _units = missionNamespace getVariable _x;

				if ([_units, [_previousUnit]] call KH_fnc_deleteArrayElements) then {
					_units pushBackUnique _newUnit;
					publicVariable _x;
				};
			} forEach ["KH_var_allPlayerUnits", "KH_var_jipPlayerUnits", "KH_var_initialPlayerUnits"];
		}
	] call KH_fnc_addEventHandler;

	[
		"CBA",
		"KH_eve_playerControlledUnitChanged",
		[], 
		{
			private _owner = param [0];
			private _unit = param [3];
			private _oldUnit = KH_var_allPlayerControlledUnitsMachines get _owner;

			if !(isNil "_oldUnit") then {
				KH_var_allPlayerControlledUnits deleteAt (KH_var_allPlayerControlledUnits find _oldUnit);
			};

			KH_var_allPlayerControlledUnitsMachines set [_owner, _unit];
			publicVariable "KH_var_allPlayerControlledUnitsMachines";
			KH_var_allPlayerControlledUnits pushBackUnique _unit;
			publicVariable "KH_var_allPlayerControlledUnits";
		}
	] call KH_fnc_addEventHandler;

	[
		"MISSION",
		"OnUserAdminStateChanged",
		[], 
		{
			params ["_networkId", "_loggedIn", "_votedIn"];
			private _machineId = _networkId getUserInfo 1;
			KH_var_adminMachine = [2, _machineId] select (_loggedIn || _votedIn);
			publicVariable "KH_var_adminMachine";
			["KH_eve_adminChanged", [_machineId, _loggedIn, _votedIn], "GLOBAL", false] call KH_fnc_triggerCbaEvent;
		}
	] call KH_fnc_addEventHandler;

	[
		"MISSION",
		"EntityRespawned",
		[],
		{
			params ["_newEntity", "_oldEntity"];
			
			{
				private _units = missionNamespace getVariable _x;

				if ([_units, [_oldEntity]] call KH_fnc_deleteArrayElements) then {
					_units pushBackUnique _newEntity;
					publicVariable _x;
				};
			} forEach ["KH_var_allPlayerUnits", "KH_var_jipPlayerUnits", "KH_var_initialPlayerUnits"];

			{
				private _entityArray = missionNamespace getVariable [_x, []];

				if (_oldEntity in _entityArray) then {
					_entityArray deleteAt (_entityArray find _oldEntity);
					_entityArray pushBackUnique _newEntity;
					missionNamespace setVariable [_x, _entityArray, true];
				};
			} forEach KH_var_entityArrayBuilderArrays;
		}
	] call KH_fnc_addEventHandler;

	[
		"MISSION",
		"EntityDeleted",
		[], 
		{
			params ["_entity"];
			
			{
				if ([missionNamespace getVariable _x, [_entity]] call KH_fnc_deleteArrayElements) then {
					publicVariable _x;
				};
			} forEach ["KH_var_allPlayerUnits", "KH_var_jipPlayerUnits", "KH_var_initialPlayerUnits", "KH_var_allHeadlessUnits", "KH_var_allPlayerControlledUnits"];
		}
	] call KH_fnc_addEventHandler;

	[
		"MISSION",
		"HandleDisconnect",
		[], 
		{
			params ["_unit", "_id", "_uid", "_name"];
			private _machineId = KH_var_allPlayerUidMachines get _uid;

			if !(isNil "_machineId") then {
				private _attributes = [];

				if !(isNull _unit) then {					
					_attributes = if (alive _unit) then {
						[_unit] call KH_fnc_getUnitAttributes;
					}
					else {
						[];
					};

					KH_var_disconnectedPlayerUids pushBackUnique _uid;
					publicVariable "KH_var_disconnectedPlayerUids";

					{
						if ([missionNamespace getVariable _x, [_unit]] call KH_fnc_deleteArrayElements) then {
							publicVariable _x;
						};
					} forEach [
						"KH_var_allPlayerUnits", 
						"KH_var_jipPlayerUnits", 
						"KH_var_initialPlayerUnits",
						"KH_var_allPlayerControlledUnits"
					];
				};

				{
					if ([missionNamespace getVariable _x, [_machineId]] call KH_fnc_deleteArrayElements) then {
						publicVariable _x;
					};
				} forEach [
					"KH_var_allMachines", 
					"KH_var_allCuratorMachines", 
					"KH_var_allPlayerMachines", 
					"KH_var_jipPlayerMachines",
					"KH_var_initialPlayerMachines"
				];

				KH_var_allIdMachines deleteAt _id;
				publicVariable "KH_var_allIdMachines";
				KH_var_allPlayerUidMachines deleteAt _uid;
				publicVariable "KH_var_allPlayerUidMachines";
				KH_var_allPlayerIdMachines deleteAt _id;
				publicVariable "KH_var_allPlayerIdMachines";
				["KH_eve_playerDisconnected", [_machineId, _uid, _id, _unit, _attributes], "GLOBAL", false] call KH_fnc_triggerCbaEvent;
			}
			else {
				private _machineId = KH_var_allHeadlessIdMachines get _id;

				if !(isNil "_machineId") then {
					{
						if ([missionNamespace getVariable _x, [_unit]] call KH_fnc_deleteArrayElements) then {
							publicVariable _x;
						};
					} forEach ["KH_var_allHeadlessUnits"];

					{
						if ([missionNamespace getVariable _x, [_machineId]] call KH_fnc_deleteArrayElements) then {
							publicVariable _x;
						};
					} forEach [
						"KH_var_allMachines", 
						"KH_var_allHeadlessMachines"
					];

					KH_var_allIdMachines deleteAt _id;
					publicVariable "KH_var_allIdMachines";
					KH_var_allHeadlessIdMachines deleteAt _id;
					publicVariable "KH_var_allHeadlessIdMachines";
				};

				["KH_eve_headlessClientDisconnected", [_machineId, _id], "GLOBAL", false] call KH_fnc_triggerCbaEvent;
			};
		}
	] call KH_fnc_addEventHandler;

	[
		[],
		{
			if KH_var_diagnosticsState then {
				missionNamespace setVariable ["KH_var_diagnosticsFramerateServer", parseNumber (diag_fps toFixed 0), KH_var_adminMachine];
				missionNamespace setVariable ["KH_var_diagnosticsLocalUnitsServer", {local _x;} count allUnits, KH_var_adminMachine];
			};

			{
				missionNamespace setVariable [["KH_var_ping_", owner _x] joinString "", ((getPlayerID _x) getUserInfo 9) select 0, true];
			} forEach KH_var_allPlayerUnits;
		},
		true,
		1,
		false
	] call KH_fnc_execute;

	[
		[],
		{
			if (KH_var_adminMachine isNotEqualTo clientOwner) then {
				{
					_x setVariable ["KH_var_playerViewDistance", _x getVariable "KH_var_playerViewDistance", KH_var_adminMachine];
					_x setVariable ["KH_var_playerAspectRatio", _x getVariable "KH_var_playerAspectRatio", KH_var_adminMachine];
					_x setVariable ["KH_var_playerCameraPosition", _x getVariable "KH_var_playerCameraPosition", KH_var_adminMachine];
					_x setVariable ["KH_var_playerCameraDirection", _x getVariable "KH_var_playerCameraDirection", KH_var_adminMachine];
					_x setVariable ["KH_var_playerUnit", _x getVariable "KH_var_playerUnit", KH_var_adminMachine];
				} forEach KH_var_allPlayerUnits;
			};
		},
		true,
		0,
		false
	] call KH_fnc_execute;
}
else {
	KH_var_clientRegistered = false;
};

if hasInterface then {
	KH_fnc_playerMissionLoadInit = {};
	KH_fnc_playerMissionStartInit = {};
	KH_fnc_playerLoadInit = {};
	KH_fnc_playerPlayersLoadedInit = {};
	KH_fnc_playerKilledInit = {};
	KH_fnc_playerRespawnInit = {};
	KH_fnc_playerSwitchInit = {};
	KH_fnc_playerControlledUnitChangeInit = {};
	KH_fnc_playerMissionEndInit = {};
	KH_var_playerMissionLoadStack = [];
	KH_var_playerMissionStartStack = [];
	KH_var_playerLoadStack = [];
	KH_var_playerPlayersLoadedStack = [];
	KH_var_playerKilledStack = [];
	KH_var_playerRespawnStack = [];
	KH_var_playerSwitchStack = [];
	KH_var_playerControlledUnitChangeStack = [];
	KH_var_playerMissionEndStack = [];
	KH_var_playerMovingObjectParent = objNull;
	KH_var_playerGeometryRoadway = objNull;

	{
		private _basePath = (getText (_x >> "path")) regexReplace ["(/)", "\\"];

		{
			private _config = _x;

			{
				if (isNumber (_config >> _x)) then {
					if ((getNumber (_config >> _x)) isEqualTo 1) then {
						(missionNamespace getVariable (["kh_var_", _x, "stack"] joinString "")) pushBack (compile (preprocessFileLineNumbers ([
							_basePath,
							["", "\"] select (_basePath isNotEqualTo ""),
							if (isText (_config >> "path")) then {
								(getText (_config >> "path")) regexReplace ["(/)", "\\"];
							}
							else {
								"";
							}, 
							configName _config, 
							".sqf"
						] joinString "")));
					};
				};
			} forEach ["playermissionload", "playermissionstart", "playerload", "playerplayersloaded", "playerkilled", "playerrespawn", "playerswitch", "playercontrolledunitchange", "playermissionend"];
		} forEach ("true" configClasses _x);
	} forEach (("true" configClasses (configFile >> "CfgKhInitFunctions")) + ("true" configClasses (missionConfigFile >> "CfgKhInitFunctions")));

	[
		["USER_ACTION", "defaultAction"],
		"Activate",
		[],
		{
			if ((KH_var_playerUnit getVariable ["KH_var_meleeMode", ""]) isNotEqualTo "") then {
				if (!dialog && !visibleMap && (isNull curatorCamera)) then {
					[KH_var_playerUnit, "ATTACK"] call KH_fnc_updateMeleeState;
				};
			};
		}
	] call KH_fnc_addEventHandler;

	[
		"PLAYER",
		"unit",
		[],
		{
			params ["_unit"];
			private _previousUnit = KH_var_playerUnit;
			KH_var_playerUnit = _unit;
			player setVariable ["KH_var_playerUnit", KH_var_playerUnit, true];
			["KH_eve_playerControlledUnitChanged", [clientOwner, getPlayerUID player, getPlayerID player, _unit, _previousUnit, [_unit, true] call KH_fnc_getEntityVariableName], "GLOBAL", false] call KH_fnc_triggerCbaEvent;

			{
				[_previousUnit, _unit] call _x;
			} forEach KH_var_playerControlledUnitChangeStack;

			[_previousUnit, _unit] call KH_fnc_playerControlledUnitChangeInit;
		},
		true
	] call KH_fnc_addEventHandler;

	[
		"MISSION",
		"TeamSwitch",
		[], 
		{
			params ["_previousUnit", "_newUnit"];

			{
				[_previousUnit] call _x;
			} forEach KH_var_playerSwitchStack;

			[_previousUnit] call KH_fnc_playerSwitchInit;
			["KH_eve_playerSwitched", [clientOwner, getPlayerUID _newUnit, getPlayerID _newUnit, _newUnit, _previousUnit], "GLOBAL", false] call KH_fnc_triggerCbaEvent;
			
			if (KH_var_playerRespawnedEventHandler isNotEqualTo []) then {
				[KH_var_playerRespawnedEventHandler] call KH_fnc_removeHandler;
			};

			KH_var_playerRespawnedEventHandler = [
				["ENTITY", _newUnit, "LOCAL"],
				"Respawn",
				[],
				{
					params ["_unit", "_corpse"];
					_corpse setVariable ["KH_var_playerUnit", _unit];
					_corpse setVehicleVarName "";

					[
						[_corpse],
						{
							params ["_corpse"];
							_corpse setVehicleVarName "";
						},
						"GLOBAL",
						true,
						true
					] call KH_fnc_execute;
					
					[_corpse] call KH_fnc_playerRespawnInit;
					["KH_eve_playerRespawned", [owner _unit, getPlayerUID _unit, getPlayerID _unit, _unit, _corpse], "GLOBAL", false] call KH_fnc_triggerCbaEvent;
					nil;
				}
			] call KH_fnc_addEventHandler;

			if (KH_var_playerKilledEventHandler isNotEqualTo []) then {
				[KH_var_playerKilledEventHandler] call KH_fnc_removeHandler;
			};

			KH_var_playerKilledEventHandler = [
				["ENTITY", _newUnit, "LOCAL"],
				"Killed",
				[],
				{
					params ["_unit", "_killer", "_instigator"];
					[_killer, _instigator] call KH_fnc_playerKilledInit;
					["KH_eve_playerKilled", [owner _unit, getPlayerUID _unit, getPlayerID _unit, _unit, _killer, _instigator], "GLOBAL", false] call KH_fnc_triggerCbaEvent;
				}
			] call KH_fnc_addEventHandler;
		}
	] call KH_fnc_addEventHandler;

	[
		"MISSION",
		"Draw2D", 
		[],
		{
			if (KH_var_drawUi2dExecutionStackAdditions isNotEqualTo []) then {
				KH_var_drawUi2dExecutionStack append KH_var_drawUi2dExecutionStackAdditions;
				KH_var_drawUi2dExecutionStackAdditions resize 0;
			};

			if (KH_var_drawUi2dExecutionStackDeletions isNotEqualTo []) then {
				{
					if ((_x select 3) in KH_var_drawUi2dExecutionStackDeletions) then {
						KH_var_drawUi2dExecutionStackDeletions set [KH_var_drawUi2dExecutionStackDeletions find (_x select 3), _forEachIndex];
					};
				} forEach KH_var_drawUi2dExecutionStack;

				KH_var_drawUi2dExecutionStack deleteAt (KH_var_drawUi2dExecutionStackDeletions select {_x isEqualType 0;});
				KH_var_drawUi2dExecutionStackDeletions resize 0;
			};

			{
				_x params ["_args", "_function", "_handlerId", "_eventName", "_previousReturn", "_executionTime"];
				_x set [4, _args call _function];
			} forEach KH_var_drawUi2dExecutionStack;
		}
	] call KH_fnc_addEventHandler;

	[
		"MISSION",
		"Draw3D", 
		[],
		{
			if (KH_var_drawUi3dOrphanExecutionStack isNotEqualTo []) then {
				{
					_x params ["_type", "_arguments"];

					switch _type do {
						case "LINE": {
							drawLine3D _arguments;
						};

						case "LASER": {
							drawLaser _arguments;
						};

						case "ICON": {
							drawIcon3D _arguments;
						};
					};
				} forEach KH_var_drawUi3dOrphanExecutionStack;

				KH_var_drawUi3dOrphanExecutionStack resize 0;
			};

			if (KH_var_drawUi3dExecutionStackAdditions isNotEqualTo []) then {
				KH_var_drawUi3dExecutionStack append KH_var_drawUi3dExecutionStackAdditions;
				KH_var_drawUi3dExecutionStackAdditions resize 0;
			};

			if (KH_var_drawUi3dExecutionStackDeletions isNotEqualTo []) then {
				{
					if ((_x select 3) in KH_var_drawUi3dExecutionStackDeletions) then {
						KH_var_drawUi3dExecutionStackDeletions set [KH_var_drawUi3dExecutionStackDeletions find (_x select 3), _forEachIndex];
					};
				} forEach KH_var_drawUi3dExecutionStack;

				KH_var_drawUi3dExecutionStack deleteAt (KH_var_drawUi3dExecutionStackDeletions select {_x isEqualType 0;});
				KH_var_drawUi3dExecutionStackDeletions resize 0;
			};

			{
				_x params ["_args", "_function", "_handlerId", "_eventName", "_previousReturn", "_executionTime"];
				_x set [4, _args call _function];
			} forEach KH_var_drawUi3dExecutionStack;
		}
	] call KH_fnc_addEventHandler;

	[
		"CBA",
		"KH_eve_drawUiExecutionStackHandler",
		[], 
		{
			params ["_handlerId", "_overrideTimeoutOnDeletion", "_conditionFailure"];
			private _currentHandler = KH_var_drawUiExecutionStackMonitor get _handlerId;
			if (isNil "_currentHandler") exitWith {};
			_currentHandler params ["_event", "_timeoutArguments", "_timeoutFunction", "_timeoutOnDeletion"];
			private _drawType = [KH_var_drawUi2dExecutionStackAdditions, KH_var_drawUi3dExecutionStackAdditions] select _event;
			private _deletionType = [KH_var_drawUi2dExecutionStackDeletions, KH_var_drawUi3dExecutionStackDeletions] select _event;

			if !(_handlerId in _deletionType) then {
				_deletionType insert [0, [_handlerId]];

				if (_timeoutOnDeletion || _overrideTimeoutOnDeletion) then {
					_timeoutArguments call _timeoutFunction;
				};
			};

			KH_var_drawUiExecutionStackMonitor deleteAt _handlerId;
		}
	] call KH_fnc_addEventHandler;

	[
		[],
		{
			if KH_var_diagnosticsState then {
				player setVariable ["KH_var_diagnosticsFramerate", parseNumber (diag_fps toFixed 0), KH_var_adminMachine];
			};
		},
		true,
		1,
		false
	] call KH_fnc_execute;

	[
		[],
		{
			player setVariable ["KH_var_playerViewDistance", viewDistance, 2];
			player setVariable ["KH_var_playerAspectRatio", getResolution select 4, 2];
			player setVariable ["KH_var_playerCameraPosition", positionCameraToWorld [0, 0, 0], 2];
			player setVariable ["KH_var_playerCameraDirection", getCameraViewDirection player, 2];

			if ((KH_var_anchorPlayersToGeometry || (KH_var_anchorPlayersToMovingObjects isNotEqualTo 0)) && (isNull (objectParent player)) && (isNull (attachedTo player)) && (alive player)) then {
				private _position = getPosASLVisual player;
				private _aimPosition = [_position select 0, _position select 1, (AGLToASL (unitAimPositionVisual player)) select 2];

				private _roofIntersection = [
					_aimPosition,
					_aimPosition vectorAdd [0, 0, 1],
					[player, "KH_InvisibleWalkableSurface_1x1x0"] + (attachedObjects player),
					true,
					1,
					"GEOM",
					"FIRE",
					true,
					[]
				] call KH_fnc_raycast;

				private _groundIntersection = [
					_aimPosition,
					_position vectorAdd [0, 0, -1],
					[player, "KH_InvisibleWalkableSurface_1x1x0"] + (attachedObjects player),
					true,
					1,
					"ROADWAY",
					"NONE",
					true,
					[]
				] call KH_fnc_raycast;

				if (_groundIntersection isEqualTo []) then {
					_groundIntersection = [
						_aimPosition,
						_position vectorAdd [0, 0, -1],
						[player, "KH_InvisibleWalkableSurface_1x1x0"] + (attachedObjects player),
						true,
						1,
						"GEOM",
						"FIRE",
						true,
						[]
					] call KH_fnc_raycast;
				};

				private _useRoadway = if (KH_var_anchorPlayersToMovingObjects isNotEqualTo 0) then {
					if ((_groundIntersection isNotEqualTo []) && (_roofIntersection isEqualTo [])) then {
						private _entity = (_groundIntersection select 0) select 3;

						if (isNull _entity) exitWith {
							if !(isNull KH_var_playerMovingObjectParent) then {
								KH_var_playerMovingObjectParent = objNull;
							};

							if ((missionNamespace getVariable ["KH_var_playerAnchorHandler", []]) isNotEqualTo []) then {
								[(missionNamespace getVariable ["KH_var_playerAnchorHandler", []])] call KH_fnc_removeHandler;
								missionNamespace setVariable ["KH_var_playerAnchorHandler", []];
							};

							true;
						};

						if (((vectorMagnitude (velocity _entity)) > 0.001) || ((vectorMagnitude (velocity (attachedTo _entity))) > 0.001)) then {
							private _attachEntity = [_entity, attachedTo _entity] select ((vectorMagnitude (velocity (attachedTo _entity))) > 0.001);

							if !(isNull KH_var_playerGeometryRoadway) then {
								KH_var_playerGeometryRoadway setPosASL [0, 0, 0];
							};
							
							if (_attachEntity isNotEqualTo KH_var_playerMovingObjectParent) then {
								KH_var_playerMovingObjectParent = _attachEntity;

								missionNamespace setVariable [
									"KH_var_playerAnchorHandler",
									[
										player, 
										_attachEntity, 
										true, 
										true, 
										1, 
										"", 
										[
											true, 
											true,
											switch KH_var_anchorPlayersToMovingObjects do {
												case 1: {
													"VELOCITY";
												};

												case 2: {
													"POSITION";
												};

												case 3: {
													"VARIABLE";
												};
											},
											true,
											true,
											1,
											true
										]
									] call KH_fnc_attach
								];
							};
							
							false;
						}
						else {
							if !(isNull KH_var_playerMovingObjectParent) then {
								KH_var_playerMovingObjectParent = objNull;
							};

							if ((missionNamespace getVariable ["KH_var_playerAnchorHandler", []]) isNotEqualTo []) then {
								[(missionNamespace getVariable ["KH_var_playerAnchorHandler", []])] call KH_fnc_removeHandler;
								missionNamespace setVariable ["KH_var_playerAnchorHandler", []];
							};

							true;
						};
					}
					else {
						if !(isNull KH_var_playerMovingObjectParent) then {
							KH_var_playerMovingObjectParent = objNull;
						};

						if ((missionNamespace getVariable ["KH_var_playerAnchorHandler", []]) isNotEqualTo []) then {
							[(missionNamespace getVariable ["KH_var_playerAnchorHandler", []])] call KH_fnc_removeHandler;
							missionNamespace setVariable ["KH_var_playerAnchorHandler", []];
						};

						true;
					};
				}
				else {
					if !(isNull KH_var_playerMovingObjectParent) then {
						KH_var_playerMovingObjectParent = objNull;
					};

					if ((missionNamespace getVariable ["KH_var_playerAnchorHandler", []]) isNotEqualTo []) then {
						[(missionNamespace getVariable ["KH_var_playerAnchorHandler", []])] call KH_fnc_removeHandler;
						missionNamespace setVariable ["KH_var_playerAnchorHandler", []];
					};

					true;
				};

				if KH_var_anchorPlayersToGeometry then {
					if (isNull KH_var_playerGeometryRoadway) then {
						KH_var_playerGeometryRoadway = createVehicleLocal ["KH_InvisibleWalkableSurface_1x1x0", [0, 0, 0], [], 0, "CAN_COLLIDE"];
					};

					if _useRoadway then {
						private _roadwayIntersection = [
							_aimPosition,
							_position vectorAdd [0, 0, -1],
							[player, "KH_InvisibleWalkableSurface_1x1x0"] + (attachedObjects player),
							true,
							1,
							"GEOM",
							"FIRE",
							true,
							[]
						] call KH_fnc_raycast;

						if ((_roadwayIntersection isNotEqualTo []) && (_roofIntersection isEqualTo [])) then {
							private _entity = (_roadwayIntersection select 0) select 3;

							if (isNull _entity) exitWith {
								KH_var_playerGeometryRoadway setPosASL [0, 0, 0];
							};

							if (((vectorMagnitude (velocity _entity)) <= 0.001) && ((vectorMagnitude (velocity (attachedTo _entity))) <= 0.001)) then {
								private _roadwayPosition = ((_roadwayIntersection select 0) select 0) vectorAdd (((_roadwayIntersection select 0) select 1) vectorMultiply 0.01);
								KH_var_playerGeometryRoadway setPosASL _roadwayPosition;
							}
							else {
								KH_var_playerGeometryRoadway setPosASL [0, 0, 0];
							};
						}
						else {
							KH_var_playerGeometryRoadway setPosASL [0, 0, 0];
						};
					}
					else {
						KH_var_playerGeometryRoadway setPosASL [0, 0, 0];
					};
				}
				else {
					if !(isNull KH_var_playerGeometryRoadway) then {
						deleteVehicle KH_var_playerGeometryRoadway;
					};
				};
			}
			else {
				if !(isNull KH_var_playerMovingObjectParent) then {
					KH_var_playerMovingObjectParent = objNull;
				};

				if ((missionNamespace getVariable ["KH_var_playerAnchorHandler", []]) isNotEqualTo []) then {
					[(missionNamespace getVariable ["KH_var_playerAnchorHandler", []])] call KH_fnc_removeHandler;
					missionNamespace setVariable ["KH_var_playerAnchorHandler", []];
				};

				if !(isNull KH_var_playerGeometryRoadway) then {
					deleteVehicle KH_var_playerGeometryRoadway;
				};
			};
		},
		true,
		0,
		false
	] call KH_fnc_execute;
};

if (!isServer && !hasInterface) then {
	KH_fnc_headlessMissionLoadInit = {};
	KH_fnc_headlessMissionStartInit = {};
	KH_fnc_headlessLoadInit = {};
	KH_fnc_headlessPlayersLoadedInit = {};
	KH_fnc_headlessMissionEndInit = {};
	KH_var_headlessMissionLoadStack = [];
	KH_var_headlessMissionStartStack = [];
	KH_var_headlessLoadStack = [];
	KH_var_headlessPlayersLoadedStack = [];
	KH_var_headlessMissionEndStack = [];


	{
		private _basePath = (getText (_x >> "path")) regexReplace ["(/)", "\\"];

		{
			private _config = _x;

			{
				if (isNumber (_config >> _x)) then {
					if ((getNumber (_config >> _x)) isEqualTo 1) then {
						(missionNamespace getVariable (["kh_var_", _x, "stack"] joinString "")) pushBack (compile (preprocessFileLineNumbers ([
							_basePath,
							["", "\"] select (_basePath isNotEqualTo ""),
							if (isText (_config >> "path")) then {
								(getText (_config >> "path")) regexReplace ["(/)", "\\"];
							}
							else {
								"";
							}, 
							configName _config, 
							".sqf"
						] joinString "")));
					};
				};
			} forEach ["headlessmissionload", "headlessmissionstart", "headlessload", "headlessplayersloaded", "headlessmissionend"];
		} forEach ("true" configClasses _x);
	} forEach (("true" configClasses (configFile >> "CfgKhInitFunctions")) + ("true" configClasses (missionConfigFile >> "CfgKhInitFunctions")));

	[
		[],
		{
			if KH_var_diagnosticsState then {
				player setVariable ["KH_var_diagnosticsFramerate", parseNumber (diag_fps toFixed 0), KH_var_adminMachine];
				player setVariable ["KH_var_diagnosticsLocalUnits", {local _x;} count allUnits, KH_var_adminMachine];
			};
		},
		true,
		1,
		false
	] call KH_fnc_execute;
};

{
	luaExecute _x;
} forEach KH_var_preInitLuaExecutions;

nil;