class Mission
{
	class Scenario
	{
		class AttributeCategories
		{
			class KH_Initialization
			{
				displayName = "KH Initialization";
				collapsed = 1;
				class Attributes
				{
					class KH_ServerMissionLoadInit
					{
						displayName = "Server: Mission Load Init";
						tooltip = "Unscheduled code to execute locally to the server when the mission loads. Players may not yet be present, and their units may not yet be available.";
						property = "KH_ServerMissionLoadInit";
						control = "EditCodeMulti5";
						expression = 
						"\
							if (_value isEqualTo '') then {\
								if (fileExists 'kh_serverMissionLoadInit.sqf') then {
									KH_fnc_serverMissionLoadInit = compileScript ['kh_serverMissionLoadInit.sqf', false, ''];
								};
							}\
							else {\
								if (!is3DEN && isServer) then {\
									KH_fnc_serverMissionLoadInit = compile _value;\
								};\
							};\
						";
						defaultValue = "''";
						validate = "expression";
					};
					class KH_ServerMissionStartInit
					{
						displayName = "Server: Mission Start Init";
						tooltip = "Unscheduled code to execute locally to the server when the mission starts. Players are present, but their units may not yet be available.";
						property = "KH_ServerMissionStartInit";
						control = "EditCodeMulti5";
						expression = 
						"\
							if (_value isEqualTo '') then {\
								if (fileExists 'kh_serverMissionStartInit.sqf') then {
									KH_fnc_serverMissionStartInit = compileScript ['kh_serverMissionStartInit.sqf', false, ''];
								};
							}\
							else {\
								if (!is3DEN && isServer) then {\
									KH_fnc_serverMissionStartInit = compile _value;\
								};\
							};\
						";
						defaultValue = "''";
						validate = "expression";
					};		
					class KH_ServerPlayersLoadedInit
					{
						displayName = "Server: Players Loaded Init";
						tooltip = "Unscheduled code to execute locally to the server once all player units have become available. This stage may be suspended for a maximum of 60 seconds before it automatically triggers in case of a player that never loads.";
						property = "KH_ServerPlayersLoadedInit";
						control = "EditCodeMulti5";
						expression = 
						"\
							if (_value isEqualTo '') then {\
								if (fileExists 'kh_serverPlayersLoadedInit.sqf') then {
									KH_fnc_serverPlayersLoadedInit = compileScript ['kh_serverPlayersLoadedInit.sqf', false, ''];
								};
							}\
							else {\
								if (!is3DEN && isServer) then {\
									KH_fnc_serverPlayersLoadedInit = compile _value;\
								};\
							};\
						";
						defaultValue = "''";
						validate = "expression";
					};
					class KH_ServerMissionEndInit
					{
						displayName = "Server: Mission End Init";
						tooltip = "Unscheduled code to execute locally to the server when the KH_fnc_endMission function or the KH End Mission module is executed.";
						property = "KH_ServerMissionEndInit";
						control = "EditCodeMulti5";
						expression = 
						"\
							if (_value isEqualTo '') then {\
								if (fileExists 'kh_serverMissionEndInit.sqf') then {
									KH_fnc_serverMissionEndInit = compileScript ['kh_serverMissionEndInit.sqf', false, ''];
								};
							}\
							else {\
								if (!is3DEN && isServer) then {\
									KH_fnc_serverMissionEndInit = compile _value;\
								};\
							};\
						";
						defaultValue = "''";
						validate = "expression";
					};
					class KH_HeadlessMissionLoadInit
					{
						displayName = "Headless: Mission Load Init";
						tooltip = "Unscheduled code to execute locally to each headless client when they load into the mission. Players may not yet be present, and their units may not yet be available.";
						property = "KH_HeadlessMissionLoadInit";
						control = "EditCodeMulti5";
						expression = 
						"\
							if (_value isEqualTo '') then {\
								if (fileExists 'kh_headlessMissionLoadInit.sqf') then {
									KH_fnc_headlessMissionLoadInit = compileScript ['kh_headlessMissionLoadInit.sqf', false, ''];
								};
							}\
							else {\
								if (!is3DEN && !isServer && !hasInterface) then {\
									KH_fnc_headlessMissionLoadInit = compile _value;\
								};\
							};\
						";
						defaultValue = "''";
						validate = "expression";
					};
					class KH_HeadlessMissionStartInit
					{
						displayName = "Headless: Mission Start Init";
						tooltip = "Unscheduled code to execute locally to each headless client when the mission starts. Players are present, but their units may not yet be available.";
						property = "KH_HeadlessMissionStartInit";
						control = "EditCodeMulti5";
						expression = 
						"\
							if (_value isEqualTo '') then {\
								if (fileExists 'kh_headlessMissionStartInit.sqf') then {
									KH_fnc_headlessMissionStartInit = compileScript ['kh_headlessMissionStartInit.sqf', false, ''];
								};
							}\
							else {\
								if (!is3DEN && !isServer && !hasInterface) then {\
									KH_fnc_headlessMissionStartInit = compile _value;\
								};\
							};\
						";
						defaultValue = "''";
						validate = "expression";
					};
					class KH_HeadlessLoadInit
					{
						displayName = "Headless: Load Init";
						tooltip = "Unscheduled code to execute locally to each headless client when their local logic unit becomes available. Players are present, but their units may not yet be available.";
						property = "KH_HeadlessLoadInit";
						control = "EditCodeMulti5";
						expression = 
						"\
							if (_value isEqualTo '') then {\
								if (fileExists 'kh_headlessLoadInit.sqf') then {
									KH_fnc_headlessLoadInit = compileScript ['kh_headlessLoadInit.sqf', false, ''];
								};
							}\
							else {\
								if (!is3DEN && !isServer && !hasInterface) then {\
									KH_fnc_headlessLoadInit = compile _value;\
								};\
							};\
						";
						defaultValue = "''";
						validate = "expression";
					};
					class KH_HeadlessPlayersLoadedInit
					{
						displayName = "Headless: Players Loaded Init";
						tooltip = "Unscheduled code to execute locally to each headless client when all player units have become available. This stage may be suspended for a maximum of 60 seconds before it automatically triggers in case of a player that never loads.";
						property = "KH_HeadlessPlayersLoadedInit";
						control = "EditCodeMulti5";
						expression = 
						"\
							if (_value isEqualTo '') then {\
								if (fileExists 'kh_headlessPlayersLoadedInit.sqf') then {
									KH_fnc_headlessPlayersLoadedInit = compileScript ['kh_headlessPlayersLoadedInit.sqf', false, ''];
								};
							}\
							else {\
								if (!is3DEN && !isServer && !hasInterface) then {\
									KH_fnc_headlessPlayersLoadedInit = compile _value;\
								};\
							};\
						";
						defaultValue = "''";
						validate = "expression";
					};
					class KH_HeadlessMissionEndInit
					{
						displayName = "Headless: Mission End Init";
						tooltip = "Unscheduled code to execute locally to each headless client when the KH_fnc_endMission function or the KH End Mission module is executed.";
						property = "KH_HeadlessMissionEndInit";
						control = "EditCodeMulti5";
						expression = 
						"\
							if (_value isEqualTo '') then {\
								if (fileExists 'kh_headlessMissionEndInit.sqf') then {
									KH_fnc_headlessMissionEndInit = compileScript ['kh_headlessMissionEndInit.sqf', false, ''];
								};
							}\
							else {\
								if (!is3DEN && !isServer && !hasInterface) then {\
									KH_fnc_headlessMissionEndInit = compile _value;\
								};\
							};\
						";
						defaultValue = "''";
						validate = "expression";
					};
					class KH_PlayerMissionLoadInit
					{
						displayName = "Player: Mission Load Init";
						tooltip = "Unscheduled code to execute locally to each player when they load into the mission. The local player unit is not yet available. Players may not yet be present, and their units may not yet be available.";
						property = "KH_PlayerMissionLoadInit";
						control = "EditCodeMulti5";
						expression = 
						"\
							if (_value isEqualTo '') then {\
								if (fileExists 'kh_playerMissionLoadInit.sqf') then {
									KH_fnc_playerMissionLoadInit = compileScript ['kh_playerMissionLoadInit.sqf', false, ''];
								};
							}\
							else {\
								if (!is3DEN && hasInterface) then {\
									KH_fnc_playerMissionLoadInit = compile _value;\
								};\
							};\
						";
						defaultValue = "''";
						validate = "expression";
					};
					class KH_PlayerMissionStartInit
					{
						displayName = "Player: Mission Start Init";
						tooltip = "Unscheduled code to execute locally to each player when the mission starts. The local player unit may not yet be available. Players are present, but their units may not yet be available.";
						property = "KH_PlayerMissionStartInit";
						control = "EditCodeMulti5";
						expression = 
						"\
							if (_value isEqualTo '') then {\
								if (fileExists 'kh_playerMissionStartInit.sqf') then {
									KH_fnc_playerMissionStartInit = compileScript ['kh_playerMissionStartInit.sqf', false, ''];
								};
							}\
							else {\
								if (!is3DEN && hasInterface) then {\
									KH_fnc_playerMissionStartInit = compile _value;\
								};\
							};\
						";
						defaultValue = "''";
						validate = "expression";
					};
					class KH_PlayerLoadInit
					{
						displayName = "Player: Load Init";
						tooltip = "Unscheduled code to execute locally to each player when their local player unit becomes available. Players are present, but their units may not yet be available.";
						property = "KH_PlayerLoadInit";
						control = "EditCodeMulti5";
						expression = 
						"\
							if (_value isEqualTo '') then {\
								if (fileExists 'kh_playerLoadInit.sqf') then {
									KH_fnc_playerLoadInit = compileScript ['kh_playerLoadInit.sqf', false, ''];
								};
							}\
							else {\
								if (!is3DEN && hasInterface) then {\
									KH_fnc_playerLoadInit = compile _value;\
								};\
							};\
						";
						defaultValue = "''";
						validate = "expression";
					};
					class KH_PlayerPlayersLoadedInit
					{
						displayName = "Player: Players Loaded Init";
						tooltip = "Unscheduled code to execute locally to each player when all player units have become available. This stage may be suspended for a maximum of 60 seconds before it automatically triggers in case of a player that never loads.";
						property = "KH_PlayerPlayersLoadedInit";
						control = "EditCodeMulti5";
						expression = 
						"\
							if (_value isEqualTo '') then {\
								if (fileExists 'kh_playerPlayersLoadedInit.sqf') then {
									KH_fnc_playerPlayersLoadedInit = compileScript ['kh_playerPlayersLoadedInit.sqf', false, ''];
								};
							}\
							else {\
								if (!is3DEN && hasInterface) then {\
									KH_fnc_playerPlayersLoadedInit = compile _value;\
								};\
							};\
						";
						defaultValue = "''";
						validate = "expression";
					};
					class KH_PlayerKilledInit
					{
						displayName = "Player: Killed Init";
						tooltip = "Unscheduled code to execute locally to each player when they die. The local player unit is the killed unit. Passed arguments available through _this are: [_killer (OBJECT), _instigator (OBJECT)].";
						property = "KH_PlayerKilledInit";
						control = "EditCodeMulti5";
						expression = 
						"\
							if (_value isEqualTo '') then {\
								if (fileExists 'kh_playerKilledInit.sqf') then {
									KH_fnc_playerKilledInit = compileScript ['kh_playerKilledInit.sqf', false, ''];
								};
							}\
							else {\
								if (!is3DEN && hasInterface) then {\
									KH_fnc_playerKilledInit = compile _value;\
								};\
							};\
						";
						defaultValue = "''";
						validate = "expression";
					};
					class KH_PlayerRespawnInit
					{
						displayName = "Player: Respawn Init";
						tooltip = "Unscheduled code to execute locally to each player when they respawn. The local player unit is the respawned unit. Passed arguments available through _this are: [_corpse (OBJECT)].";
						property = "KH_PlayerRespawnInit";
						control = "EditCodeMulti5";
						expression = 
						"\
							if (_value isEqualTo '') then {\
								if (fileExists 'kh_playerRespawnInit.sqf') then {
									KH_fnc_playerRespawnInit = compileScript ['kh_playerRespawnInit.sqf', false, ''];
								};
							}\
							else {\
								if (!is3DEN && hasInterface) then {\
									KH_fnc_playerRespawnInit = compile _value;\
								};\
							};\
						";
						defaultValue = "''";
						validate = "expression";
					};
					class KH_PlayerSwitchInit
					{
						displayName = "Player: Switch Init";
						tooltip = "Unscheduled code to execute locally to each player when they switch to another unit or team. The local player unit is the switched unit. Passed arguments available through _this are: [_previousUnit (OBJECT)].";
						property = "KH_PlayerSwitchInit";
						control = "EditCodeMulti5";
						expression = 
						"\
							if (_value isEqualTo '') then {\
								if (fileExists 'kh_playerSwitchInit.sqf') then {
									KH_fnc_playerSwitchInit = compileScript ['kh_playerSwitchInit.sqf', false, ''];
								};
							}\
							else {\
								if (!is3DEN && hasInterface) then {\
									KH_fnc_playerSwitchInit = compile _value;\
								};\
							};\
						";
						defaultValue = "''";
						validate = "expression";
					};
					class KH_PlayerControlledUnitChangeInit
					{
						displayName = "Player: Controlled Unit Change Init";
						tooltip = "Unscheduled code to execute locally to each player when their controlled unit changes, such as during remote control. The local player unit remains the original player unit. This function will also activate as soon as the local player unit becomes available. Passed arguments available through _this are: [_previousUnit (OBJECT), _currentUnit (OBJECT)].";
						property = "KH_PlayerControlledUnitChangeInit";
						control = "EditCodeMulti5";
						expression = 
						"\
							if (_value isEqualTo '') then {\
								if (fileExists 'kh_playerControlledUnitChangeInit.sqf') then {
									KH_fnc_playerControlledUnitChangeInit = compileScript ['kh_playerControlledUnitChangeInit.sqf', false, ''];
								};
							}\
							else {\
								if (!is3DEN && hasInterface) then {\
									KH_fnc_playerControlledUnitChangeInit = compile _value;\
								};\
							};\
						";
						defaultValue = "''";
						validate = "expression";
					};
					class KH_PlayerMissionEndInit
					{
						displayName = "Player: Mission End Init";
						tooltip = "Unscheduled code to execute locally to each player before the mission ends when the KH_fnc_endMission function or the KH End Mission module is executed.";
						property = "KH_PlayerMissionEndInit";
						control = "EditCodeMulti5";
						expression = 
						"\
							if (_value isEqualTo '') then {\
								if (fileExists 'kh_playerMissionEndInit.sqf') then {
									KH_fnc_playerMissionEndInit = compileScript ['kh_playerMissionEndInit.sqf', false, ''];
								};
							}\
							else {\
								if (!is3DEN && hasInterface) then {\
									KH_fnc_playerMissionEndInit = compile _value;\
								};\
							};\
						";
						defaultValue = "''";
						validate = "expression";
					};
				};
			};
			class KH_Miscellaneous
			{
				displayName = "KH Miscellaneous";
				collapsed = 1;
				class Attributes
				{
					class KH_QuickFunctions
					{
						displayName = "Quick Functions";
						tooltip = "Hashmap style arrays where the first value is the name of the function, and the second value is the path to the function file which can either be SQF or Lua. The function will be compiled and defined in both the missionNamespace and uiNamespace if it is an SQF function, and into Lua global variables if it is a Lua function. These functions will then become available for execution in the KH debug console for quick and easy access during the mission.";
						property = "KH_QuickFunctions";
						control = "EditMulti5";
						expression = 
						"\
							if ((_value isNotEqualTo '') && !is3DEN) then {\
								{\
									_x params ['_name', '_function'];\
									if ('.sqf' in _function) then {\
										_function = compile (preprocessFileLineNumbers _function);\
										missionNamespace setVariable [_name, _function];\
										uiNamespace setVariable [_name, _function];\
										KH_var_quickFunctionsSqf set [_name, _function];\
									}\
									else {\
										_name luaCompile (preprocessFile _function);\
										KH_var_quickFunctionsLua set [_name, _function];\
									};\
								} forEach (parseSimpleArray (['[', _value, ']'] joinString ''));\
								uiNamespace setVariable ['KH_var_quickFunctionsSqf', KH_var_quickFunctionsSqf];\
								uiNamespace setVariable ['KH_var_quickFunctionsLua', KH_var_quickFunctionsLua];\
							};\
						";
						defaultValue = "''";
					};
				};
			};
			class KH_CameraSequence
			{
				displayName = "KH Camera Sequence";
				collapsed = 1;
				class Attributes
				{
					class KH_CameraSequenceSubcategory
					{
						description = "Plays a sequence of camera shots for players present at the start of the mission, and optionally JIP players. The attributes of each camera are interpolated based on index value equivalents from the different arrays of attributes. For example, the third element of the Positions attribute will take effect on the same shot as the third element of the Targets attribute.";
						data = "AttributeSystemSubcategory";
						control = "KH_SubcategoryNoHeader4";
					};
					class KH_CameraSequence 
					{
						property = "KH_CameraSequence";
						control = "KH_CameraSequence";
						expression = 
						"\
							_value params ['_toggle', '_positions', '_targets', '_fovs', '_commitTimes', '_durations', '_visionTypes', '_cinematicBorders', '_disableUserInput', '_jip'];\
							_positions = ['[', _positions, ']'] joinString '';\
							_targets = ['[', _targets, ']'] joinString '';\
							_fovs = ['[', _fovs, ']'] joinString '';\
							_commitTimes = ['[', _commitTimes, ']'] joinString '';\
							_durations = ['[', _durations, ']'] joinString '';\
							_visionTypes = ['[', _visionTypes, ']'] joinString '';\
							if (_toggle && !is3DEN && isServer) then {\
								private _parsedPositions = [];\
								private _parsedTargets = [];\
								{\
									private _position = if (_x isEqualType []) then {\
										if ((_x select 0) isEqualType objNull) then {\
											[missionNamespace getVariable [_x, objNull], _x select 1];\
										}\
										else {\
											_x;\
										};\
									}\
									else {\
										missionNamespace getVariable [_x, objNull];\
									};,\
									_parsedPositions pushBack _position;\
								} forEach (parseSimpleArray _positions);\
								{\
									private _target = if (_x isEqualType []) then {\
										_x;\
									}\
									else {\
										missionNamespace getVariable [_x, objNull];\
									};\
									_parsedTargets pushBack _target;\
								} forEach (parseSimpleArray _targets);\
								KH_var_postInitExecutions pushBack [\
									[_parsedPositions, _parsedTargets, parseSimpleArray _fovs, parseSimpleArray _commitTimes, parseSimpleArray _durations, parseSimpleArray _visionTypes, _cinematicBorders, _disableUserInput, _jip],\
									{\
										[\
											_this,\
											{\
												[\
													'CBA',\
													'KH_eve_playerLoaded',\
													_this,\
													{\
														private _unit = param [3];\
														_args params ['_positions', '_targets', '_fovs', '_commitTimes', '_durations', '_visionTypes', '_cinematicBorders', '_disableUserInput', '_jip'];\
														if _jip then {\
															[[_positions, _targets, _fovs, _commitTimes, _durations, _visionTypes, _cinematicBorders, _disableUserInput], 'KH_fnc_cameraSequence', _unit, true, false] call KH_fnc_execute;\
														}\
														else {\
															if !KH_var_playersLoaded then {\
																[[_positions, _targets, _fovs, _commitTimes, _durations, _visionTypes, _cinematicBorders, _disableUserInput], 'KH_fnc_cameraSequence', _unit, true, false] call KH_fnc_execute;\
															}\
															else {\
																[_handlerId] call KH_fnc_removeHandler;\
															};\
														};\
													}\
												] call KH_fnc_addEventHandler;\
											},\
											true,\
											[{KH_var_playersLoaded;}, true] select (isNil 'KH_var_missionStartSuspension'),\
											false\
										] call KH_fnc_execute;\
									}\
								];\
							};\
						";
						defaultValue = "[false, '', '', '', '', '', '', false, false, false]";
					};
				};
			};
			class KH_Curators
			{
				displayName = "KH Curators";
				collapsed = 1;
				class Attributes
				{
					class KH_CuratorsSubcategory
					{
						description = "Automatically sets curators based on index pair equivalents of strings of Steam IDs and curator module variable names. If a Steam ID does not have a module pair, a module will be created for that player.";
						data = "AttributeSystemSubcategory";
						control = "KH_SubcategoryNoHeader1";
					};
					class KH_Curators
					{
						property = "KH_Curators";
						control = "KH_Curators";
						expression = 
						"\
							_value params ['_toggle', '_curators', '_curatorModules', '_hide', '_disableDamage'];\
							_curators = ['[', _curators, ']'] joinString '';\
							_curatorModules = ['[', _curatorModules, ']'] joinString '';\
							if (_toggle && !is3DEN && isServer) then {\
								private _assignedModules = [];\
								{\
									private _module = missionNamespace getVariable [_x, objNull];\
									if !(isNull _module) then {\
										_assignedModules pushBack _module;\
									};\
								} forEach (parseSimpleArray _curatorModules);\
								[parseSimpleArray _curators, _assignedModules, _hide, _disableDamage] call KH_fnc_setCurators;\
							};\
						";
						defaultValue = "[false, str (profileNamespace getVariable ['KH_var_steamId', '']), '', true, true]";
					};
				};
			};
			class KH_Persistency
			{
				displayName = "KH Persistency";
				collapsed = 1;
				class Attributes
				{
					class KH_PersistencySubcategory
					{
						description = "Maintain persistency using stored entity and world states. States are saved when the KH_fnc_endMission function or the KH End Mission module is executed, and loaded at the appropriate time if the persistent state of the identifier is available. If only the respawn loadout usage is desired, the identifier can be left empty, or all of the options can be unticked. The missionNamespace, as well as all entity namespaces, may contain a 'KH_var_persistentVariables' variable that is an array of arrays in format [name (STRING), value (BOOL, SCALAR, STRING, ARRAY, HASHMAP), public (BOOL)].";
						data = "AttributeSystemSubcategory";
						control = "KH_SubcategoryNoHeader6";
					};
					class KH_Persistency 
					{
						property = "KH_Persistency";
						control = "KH_Persistency";
						expression = 
						"\
							_value params ['_toggle', '_identifier', '_players', '_playerRespawnType', '_units', '_groups', '_objects', '_mission'];\
							if (_toggle && !is3DEN && isServer) then {\
								private _respawnType = switch _playerRespawnType do {\
									case 1: {\
										'SAVED';\
									};\
									case 2: {\
										'INITIAL';\
									};\
									case 3: {\
										'DEATH';\
									};\
									default {\
										'NONE';\
									};\
								};\
								if _objects then {\
									[_identifier, []] call KH_fnc_loadObjectPersistency;\
								};\
								if _units then {\
									[_identifier, []] call KH_fnc_loadUnitPersistency;\
								};\
								if _players then {\
									[_identifier, _respawnType, []] call KH_fnc_loadPlayerPersistency;\
								}\
								else {\
									['', _respawnType, []] call KH_fnc_loadPlayerPersistency;\
								};\
								if _groups then {\
									[_identifier, []] call KH_fnc_loadGroupPersistency;\
								};\
								if _mission then {\
									[_identifier] call KH_fnc_loadMissionPersistency;\
								};\
								[\
									'CBA',\
									'KH_eve_missionEnded',\
									[_identifier, _players, _units, _groups, _objects, _mission],\
									{\
										_args params ['_identifier', '_players', '_units', '_groups', '_objects', '_mission'];\
										if _objects then {\
											[_identifier, entities [[], ['Man'], false, true]] call KH_fnc_saveObjectPersistency;\
										};\
										if _units then {\
											[_identifier, allUnits + allDeadMen] call KH_fnc_saveUnitPersistency;\
										};\
										if _players then {\
											[_identifier, KH_var_allPlayerUnits] call KH_fnc_savePlayerPersistency;\
										};\
										if _groups then {\
											[_identifier, allGroups] call KH_fnc_saveGroupPersistency;\
										};\
										if _mission then {\
											call KH_fnc_saveMissionPersistency;\
										};\
										[_handlerId] call KH_fnc_removeHandler;\
									}\
								] call KH_fnc_addEventHandler;\
							};\
						";
						defaultValue = "[false, profileName, true, 2, true, true, true, true]";
					};
				};
			};
			class KH_SideRelations
			{
				displayName = "KH Side Relations";
				collapsed = 1;
				class Attributes
				{
					class KH_SideRelationsSubcategory
					{
						description = "Set the relationship state of the sides, with true designating friendly and false designating hostile.";
						data = "AttributeSystemSubcategory";
						control = "KH_SubcategoryNoHeader1";
					};
					class KH_SideRelations
					{
						property = "KH_SideRelations";
						control = "KH_SideRelations";
						expression = 
						"\
							_value params ['_toggle', '_bluforToBlufor', '_bluforToOpfor', '_bluforToGreenfor', '_bluforToCivilian', '_opforToBlufor', '_opforToOpfor', '_opforToGreenfor', '_opforToCivilian', '_greenforToBlufor', '_greenforToOpfor', '_greenforToGreenfor', '_greenforToCivilian', '_civilianToBlufor', '_civilianToOpfor', '_civilianToGreenfor', '_civilianToCivilian'];\
							if (_toggle && !is3DEN && isServer) then {\
								KH_var_initialSideRelations = [[west, west, _bluforToBlufor], [west, east, _bluforToOpfor], [west, resistance, _bluforToGreenfor], [west, civilian, _bluforToCivilian], [east, west, _opforToBlufor], [east, east, _opforToOpfor], [east, resistance, _opforToGreenfor], [east, civilian, _opforToCivilian], [resistance, west, _greenforToBlufor], [resistance, east, _greenforToOpfor], [resistance, resistance, _greenforToGreenfor], [resistance, civilian, _greenforToCivilian], [civilian, west, _civilianToBlufor], [civilian, east, _civilianToOpfor], [civilian, resistance, _civilianToGreenfor], [civilian, civilian, _civilianToCivilian]];\
							};\
						";
						defaultValue = "[false, true, false, true, true, false, true, false, true, true, false, true, true, true, true, true, true]";
					};
				};
			};
		};
	};
	class Multiplayer
	{
		class AttributeCategories
		{
			class KH_Miscellaneous
			{
				displayName = "KH Miscellaneous";
				collapsed = 1;
				class Attributes
				{
					class KH_MissionStartSuspension
					{
						displayName = "Mission Start Suspension";
						tooltip = "Suspends the mission until all players have loaded in.";
						property = "KH_MissionStartSuspension";
						control = "Combo";
						expression = 
						"\
							if ((_value isNotEqualTo 0) && !is3DEN && isServer) then {\
								KH_var_missionStartSuspension = _value;\
							};\
						";
						defaultValue = "0";
						typeName = "NUMBER";
						class Values
						{
							class KH_None
							{
								name = "NONE";
								tooltip = "Prevents this function from executing.";
								value = "0";
							};
							class KH_Players
							{
								name = "PLAYERS";
								tooltip = "Disables player input and displays a suspension message to them until all players are loaded.";
								value = "1";
							};
							class KH_All
							{
								name = "ALL";
								tooltip = "Disables player input, displays a suspension message to them, and disables simulation on all units until all players are loaded.";
								value = "2";
							};
						};
					};
					class KH_PlayerSynchronizer
					{
						displayName = "Player Synchronizer";
						tooltip = "Hashmap style arrays where the key is a string of the variable name of an entity, and the value is an array of strings of variable names of players which are to be synchronized to the entity assigned to the key. Ideal for making sure modules are synchronized to respawning or JIP players, as well as initial players at the start of the mission.";
						property = "KH_PlayerSynchronizer";
						control = "EditMulti5";
						expression = 
						"\
							if ((_value isNotEqualTo '') && !is3DEN && isServer) then {\
								_value = ['[', _value, ']'] joinString '';\
								KH_var_postInitExecutions pushBack [\
									[createHashMapFromArray (parseSimpleArray _value)],\
									{\
										params ['_value'];\
										{\
											[\
												'CBA',\
												_x,\
												[_value],\
												{\
													private _unit = param [3];\
													_args params ['_value'];\
													{\
														private _parsedEntities = [];\
														{\
															_parsedEntities pushBack (missionNamespace getVariable [_x, objNull]);\
														} forEach _y;\
														(missionNamespace getVariable [_x, objNull]) synchronizeObjectsAdd _parsedEntities;\
													} forEach _value;\
												}\
											] call KH_fnc_addEventHandler;\
										} forEach ['KH_eve_playerLoaded', 'KH_eve_playerRespawned'];\
									}\
								];\
							};\
						";
						defaultValue = "''";
					};
					class KH_RecoverDisconnectedPlayers
					{
						displayName = "Recover Disconnected Players";
						tooltip = "Recovers the unit attributes of players who reconnect after disconnecting.";
						property = "KH_RecoverDisconnectedPlayers";
						control = "Checkbox";
						expression = 
						"\
							if (_value && !is3DEN && isServer) then {\
								[true] call KH_fnc_recoverDisconnectedPlayers;\
							};\
						";
						defaultValue = "false";
					};
				};
			};
		};
	};
	class Intel
	{
		class AttributeCategories
		{
			class KH_EnvironmentAmbiance
			{
				displayName = "KH Environment Ambiance";
				collapsed = 1;
				class Attributes
				{
					class KH_EnvironmentAmbianceSubcategory
					{
						description = "Determines the amount of environmental ambiance present in the mission.";
						data = "AttributeSystemSubcategory";
						control = "KH_SubcategoryNoHeader1";
					};
					class KH_EnvironmentAmbiance 
					{
						property = "KH_EnvironmentAmbiance";
						control = "KH_EnvironmentAmbiance";
						expression = 
						"\
							_value params ['_toggle', '_ambientLife', '_ambientSounds', '_windCoefficient'];\
							if (_toggle && !is3DEN) then {\
								KH_var_postInitExecutions pushBack [\
									[_ambientLife, _ambientSounds, _windCoefficient],\
									{\
										enableEnvironment _this;\
									},\
									1\
								];\
							};\
						";
						defaultValue = "[false, true, true, 1, '100%']";
					};
				};
			};
		};
	};
	class GarbageCollection
	{
		class AttributeCategories
		{
			class KH_Miscellaneous
			{
				displayName = "KH Miscellaneous";
				collapsed = 1;
				class Attributes
				{
					class KH_LimitViewDistance
					{
						displayName = "Limit View Distance";
						tooltip = "Limits the maximum view distance to the set amount for all players, preventing them from increasing it beyond the desired distance. Negative values will prevent players from decreasing it below the desired distance. May override custom view distance mods. Set to 0 for no view distance limit. Can be edited with KH_fnc_limitViewDistance.";
						property = "KH_LimitViewDistance";
						control = "Edit";
						expression = 
						"\
							if ((_value isNotEqualTo 0) && !is3DEN && isServer) then {\
								KH_var_postInitExecutions pushBack [\
									[true, _value],\
									{\
										call KH_fnc_limitViewDistance;\
									}\
								];\
							};\
						";
						defaultValue = "0";
						validate = "number";
						typeName = "NUMBER";
					};
				};
			};
		};
	};
};