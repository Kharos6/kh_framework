class Mission
{
	class Scenario
	{
		class AttributeCategories
		{
			class KH_Init
			{
				displayName = "KH Initialization";
				collapsed = 1;
				class Attributes
				{
					class KH_ServerMissionLoadInit
					{
						displayName = "Server: Mission Load Init";
						tooltip = "Unscheduled code to execute on the server when the mission loads, before the mission starts. Players may not yet be present, and their units are not yet available. Remote execution with <KH_fnc_execute> will only reliably work if <'JIP'> is used.";
						property = "KH_ServerMissionLoadInit";
						control = "EditMulti5";
						expression = 
						"\
							if ((_value != '') && !is3DEN && isServer) then {\
								KH_fnc_serverMissionLoadInit = compile _value;\
							}\
							else {\
								KH_fnc_serverMissionLoadInit = {};\
							};\
						";
						defaultValue = "''";
					};
					class KH_ServerMissionStartInit
					{
						displayName = "Server: Mission Start Init";
						tooltip = "Unscheduled code to execute on the server when the mission starts. Players are present, but their units may not yet be available. Remote execution with <KH_fnc_execute> at this stage may be used with <'JIP'> with <_unitRequired> set to <true> to ensure proper execution on player units.";
						property = "KH_ServerMissionStartInit";
						control = "EditMulti5";
						expression = 
						"\
							if ((_value != '') && !is3DEN && isServer) then {\
								KH_fnc_serverMissionStartInit = compile _value;\
							}\
							else {\
								KH_fnc_serverMissionStartInit = {};\
							};\
						";
						defaultValue = "''";
					};		
					class KH_ServerPlayersLoadedInit
					{
						displayName = "Server: Players Loaded Init";
						tooltip = "Unscheduled code to execute on the server once all player units have become available. All forms of remote execution with <KH_fnc_execute> are viable, both during and after this stage. While extremely unlikely, in the event that a player bugs out in some way, this stage will be suspended for a maximum of 60 seconds before it automatically triggers.";
						property = "KH_ServerPlayersLoadedInit";
						control = "EditMulti5";
						expression = 
						"\
							if ((_value != '') && !is3DEN && isServer) then {\
								KH_fnc_serverPlayersLoadedInit = compile _value;\
							}\
							else {\
								KH_fnc_serverPlayersLoadedInit = {};\
							};\
						";
						defaultValue = "''";
					};
					class KH_ServerMissionEndInit
					{
						displayName = "Server: Mission End Init";
						tooltip = "Unscheduled code to execute on the server when the <KH_fnc_endMission> function, or <End Mission> module, is executed.";
						property = "KH_ServerMissionEndInit";
						control = "EditMulti5";
						expression = 
						"\
							if ((_value != '') && !is3DEN && isServer) then {\
								KH_fnc_serverMissionEndInit = compile _value;\
							}\
							else {\
								KH_fnc_serverMissionEndInit = {};\
							};\
						";
						defaultValue = "''";
					};
					class KH_HeadlessLoadInit
					{
						displayName = "Headless: Load Init";
						tooltip = "Unscheduled code to execute on headless clients, both if they join while the mission is in progress, and before the mission starts if they are already present at that time. In case of execution prior to mission start, players may not yet be present, and their units are not yet available, and remote execution will only reliably work if <KH_fnc_execute> is used with <'JIP'>.";
						property = "KH_HeadlessPreInit";
						control = "EditMulti5";
						expression = 
						"\
							if ((_value != '') && !is3DEN && isServer) then {\
								KH_fnc_headlessLoadInit = compile _value;\
							}\
							else {\
								KH_fnc_headlessLoadInit = {};\
							};\
						";
						defaultValue = "''";
					};
					class KH_HeadlessMissionStartInit
					{
						displayName = "Headless: Mission Start Init";
						tooltip = "Unscheduled code to execute on headless clients when the mission starts. Players are present, but their units may not yet be available. Remote execution with <KH_fnc_execute> at this stage may be used with <'JIP'> with <_unitRequired> set to <true> to ensure proper execution on player units.";
						property = "KH_HeadlessMissionStartInit";
						control = "EditMulti5";
						expression = 
						"\
							if ((_value != '') && !is3DEN && isServer) then {\
								KH_fnc_headlessMissionStartInit = compile _value;\
							}\
							else {\
								KH_fnc_headlessMissionStartInit = {};\
							};\
						";
						defaultValue = "''";
					};
					class KH_HeadlessMissionEndInit
					{
						displayName = "Headless: Mission End Init";
						tooltip = "Unscheduled code to execute on headless clients when the the <KH_fnc_endMission> function, or <End Mission> module, function, or <End Mission> module, is executed.";
						property = "KH_HeadlessMissionEndInit";
						control = "EditMulti5";
						expression = 
						"\
							if ((_value != '') && !is3DEN && isServer) then {\
								KH_fnc_headlessMissionEndInit = compile _value;\
							}\
							else {\
								KH_fnc_headlessMissionEndInit = {};\
							};\
						";
						defaultValue = "''";
					};
					class KH_PlayerPreloadInit
					{
						displayName = "Player: Preload Init";
						tooltip = "Unscheduled code to execute locally to each player that is present before the mission starts. The local player unit is not yet available. Other players may not yet be present, and their units are not yet available, so remote execution will only reliably work if <KH_fnc_execute> is used with <'JIP'>. Passed arguments available through <_this> are: <[_uid (STRING)]>.";
						property = "KH_PlayerPreloadInit";
						control = "EditMulti5";
						expression = 
						"\
							if ((_value != '') && !is3DEN && isServer) then {\
								KH_fnc_playerPreloadInit = compile _value;\
							}\
							else {\
								KH_fnc_playerPreloadInit = {};\
							};\
						";
						defaultValue = "''";
					};
					class KH_PlayerLoadInit
					{
						displayName = "Player: Load Init";
						tooltip = "Unscheduled code to execute locally to each player that is present during the mission start, once their local player unit is available. This is the ideal stage to execute code that requires the local player unit. Other players are present, but their units may not yet be available. Remote execution with <KH_fnc_execute> at this stage may be used with <'JIP'> with <_unitRequired> set to <true> to ensure proper execution on other player units.";
						property = "KH_PlayerLoadInit";
						control = "EditMulti5";
						expression = 
						"\
							if ((_value != '') && !is3DEN && isServer) then {\
								KH_fnc_playerLoadInit = compile _value;\
							}\
							else {\
								KH_fnc_playerLoadInit = {};\
							};\
						";
						defaultValue = "''";
					};
					class KH_PlayerPlayersLoadedInit
					{
						displayName = "Player: Players Loaded Init";
						tooltip = "Unscheduled code to execute locally to each player once all other player units have become available. All forms of remote execution with <KH_fnc_execute> are viable, both during and after this stage. While extremely unlikely, in the event that a player bugs out in some way, this stage will be suspended for a maximum of 60 seconds before it automatically triggers.";
						property = "KH_PlayerPlayersLoadedInit";
						control = "EditMulti5";
						expression = 
						"\
							if ((_value != '') && !is3DEN && isServer) then {\
								KH_fnc_playerPlayersLoadedInit = compile _value;\
							}\
							else {\
								KH_fnc_playerPlayersLoadedInit = {};\
							};\
						";
						defaultValue = "''";
					};
					class KH_PlayerKilledInit
					{
						displayName = "Player: Killed Init";
						tooltip = "Unscheduled code to execute locally to each player upon their death. The local player unit is the killed unit.";
						property = "KH_PlayerKilledInit";
						control = "EditMulti5";
						expression = 
						"\
							if ((_value != '') && !is3DEN && isServer) then {\
								KH_fnc_playerKilledInit = compile _value;\
							}\
							else {\
								KH_fnc_playerKilledInit = {};\
							};\
						";
						defaultValue = "''";
					};
					class KH_PlayerRespawnInit
					{
						displayName = "Player: Respawn Init";
						tooltip = "Unscheduled code to execute locally to each player when they respawn. The local player unit is the respawned unit. Passed arguments available through <_this> are: <[_corpse (OBJECT)]>.";
						property = "KH_PlayerRespawnInit";
						control = "EditMulti5";
						expression = 
						"\
							if ((_value != '') && !is3DEN && isServer) then {\
								KH_fnc_playerRespawnInit = compile _value;\
							}\
							else {\
								KH_fnc_playerRespawnInit = {};\
							};\
						";
						defaultValue = "''";
					};
					class KH_PlayerSwitchInit
					{
						displayName = "Player: Switch Init";
						tooltip = "Unscheduled code to execute locally to each player when they switch to another unit. The local player unit is the switched unit. Passed arguments available through <_this> are: <[_previousUnit (OBJECT)]>.";
						property = "KH_PlayerSwitchInit";
						control = "EditMulti5";
						expression = 
						"\
							if ((_value != '') && !is3DEN && isServer) then {\
								KH_fnc_playerSwitchInit = compile _value;\
							}\
							else {\
								KH_fnc_playerSwitchInit = {};\
							};\
						";
						defaultValue = "''";
					};
					class KH_PlayerJIPPreloadInit
					{
						displayName = "Player: JIP Preload Init";
						tooltip = "Unscheduled code to execute locally to each player that joins while the mission is considered to be in progress, which is after the <Players Initialized Init> stage. The local player unit is not yet available. Passed arguments available through <_this> are: <[_uid (STRING)]>.";
						property = "KH_PlayerJIPPreloadInit";
						control = "EditMulti5";
						expression = 
						"\
							if ((_value != '') && !is3DEN && isServer) then {\
								KH_fnc_playerJipPreloadInit = compile _value;\
							}\
							else {\
								KH_fnc_playerJipPreloadInit = {};\
							};\
						";
						defaultValue = "''";
					};
					class KH_PlayerJIPLoadInit
					{
						displayName = "Player: JIP Load Init";
						tooltip = "Unscheduled code to execute locally to each player that joins while the mission is considered to be in progress, which is after the <Players Initialized Init> stage, once their local player unit is available.";
						property = "KH_PlayerJIPLoadInit";
						control = "EditMulti5";
						expression = 
						"\
							if ((_value != '') && !is3DEN && isServer) then {\
								KH_fnc_playerJipLoadInit = compile _value;\
							}\
							else {\
								KH_fnc_playerJipLoadInit = {};\
							};\
						";
						defaultValue = "''";
					};
					class KH_PlayerMissionEndInit
					{
						displayName = "Player: Mission End Init";
						tooltip = "Unscheduled code to execute locally to each player before the mission ends when the <KH_fnc_endMission> function, or <End Mission> module, is executed.";
						property = "KH_PlayerMissionEndInit";
						control = "EditMulti5";
						expression = 
						"\
							if ((_value != '') && !is3DEN && isServer) then {\
								KH_fnc_playerMissionEndInit = compile _value;\
							}\
							else {\
								KH_fnc_playerMissionEndInit = {};\
							};\
						";
						defaultValue = "''";
					};
				};
			};
			class KH_Miscellaneous
			{
				displayName = "KH Miscellaneous";
				collapsed = 1;
				class Attributes
				{
					class KH_PublicFunctions
					{
						displayName = "Public Functions";
						tooltip = "Set one or more mission functions, in form of strings of their name and file, to be made available to all clients, in format: <[['name1', 'file1'], ['name2', 'file2'], ['name3', 'file3']]>.";
						property = "KH_PublicFunctions";
						control = "EditMulti5";
						expression = 
						"\
							if ((_value != '') && (_value != '[]') && !is3DEN && isServer) then {\
								private _functions = createHashMapFromArray (parseSimpleArray _value);\
								{\
									missionNamespace setVariable [_x, (compile (preprocessFileLineNumbers _y)), true];\
								} forEach _functions;\
							};\
						";
						defaultValue = "'[]'";
					};
					class KH_SimpleIntroText
					{
						displayName = "Simple Intro Text";
						tooltip = "Displays text to the players when they join the mission. Leave empty for no intro text.";
						property = "KH_SimpleIntroText";
						control = "Edit";
						expression = 
						"\
							if ((_value != '') && !is3DEN) then {\
								[\
									{\
										(!(isNull player) && (alive player) && !visibleMap);\
									},\
									{\
										params ['_text'];\
										titleText [_text, 'BLACK IN', 999999];\
										[\
											{\
												params ['_text'];\
												titleText [_text, 'BLACK IN', 6];\
											},\
											[_text],\
											3\
										] call CBA_fnc_waitAndExecute;\
									},\
									[_value]\
								] call CBA_fnc_waitUntilAndExecute;\
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
						description = "Plays a sequence of camera shots for players present at the start of the mission, and optionally JIP players. The attributes of each camera are interpolated based on index value equivalents from the different arrays of attributes. For example, the third element in <Positions> will take effect on the same shot as the third element in <Targets>.";
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
							if (_toggle && !is3DEN && isServer) then {\
								private _parsedPositions = [];\
								private _parsedTargets = [];\
								{\
									private _position = '';\
									if ((_x select 0) isEqualType []) then {\
										_position = _x select 0;\
									}\
									else {\
										_position = missionNamespace getVariable [_x select 0, objNull];\
									};\
									private _attach = _x select 1;\
									_parsedPositions pushBack [_position, _attach];\
								} forEach (parseSimpleArray _positions);\
								{\
									private _target = '';\
									if (_x isEqualType []) then {\
										_target = _x;\
									}\
									else {\
										_target = missionNamespace getVariable [_x, objNull];\
									};\
									_parsedTargets pushBack _target;\
								} forEach (parseSimpleArray _targets);\
								[\
									'CBA',\
									'KH_eve_playerLoaded',\
									[_parsedPositions, _parsedTargets, parseSimpleArray _fovs, parseSimpleArray _commitTimes, parseSimpleArray _durations, parseSimpleArray _visionTypes, _cinematicBorders, _disableUserInput, _jip],\
									{\
										params ['_unit'];\
										_args params ['_positions', '_targets', '_fovs', '_commitTimes', '_durations', '_visionTypes', '_cinematicBorders', '_disableUserInput', '_jip'];\
										if _jip then {\
											[[_positions, _targets, _fovs, _commitTimes, _durations, _visionTypes, _cinematicBorders, _disableUserInput], 'KH_fnc_cameraSequence', _unit, 'THIS_FRAME'] call KH_fnc_execute;\
										}\
										else {\
											if !KH_var_playersLoaded then {\
												[[_positions, _targets, _fovs, _commitTimes, _durations, _visionTypes, _cinematicBorders, _disableUserInput], 'KH_fnc_cameraSequence', _unit, 'THIS_FRAME'] call KH_fnc_execute;\
											}\
											else {\
												[_eventName, _localId] call CBA_fnc_removeEventHandler;\
											};\
										};\
									}\
								] call KH_fnc_addEventHandler;\
							};\
						";
						defaultValue = "[false, '[]', '[]', '[]', '[]', '[]', '[]', false, false, false]";
					};
				};
			};
			class KH_CaptureVehicleSequence
			{
				displayName = "KH Capture Vehicle Sequence";
				collapsed = 1;
				class Attributes
				{
					class KH_CaptureVehicleSequenceSubcategory
					{
						description = "Captures the movement, and optionally weapon fire, of a vehicle, then saves the output to the clipboard.";
						data = "AttributeSystemSubcategory";
						control = "KH_SubcategoryNoHeader2";
					};
					class KH_CaptureVehicleSequence 
					{
						property = "KH_CaptureVehicleSequence";
						control = "KH_CaptureVehicleSequence";
						expression = 
						"\
							_value params ['_toggle', '_vehicle', '_weaponFire', '_waitTime', '_duration', '_framerate'];\
							if (_toggle && !is3DEN && isServer && (_vehicle != '')) then {\
								private _assignedVehicle = missionNamespace getVariable [_vehicle, objNull];\
								if !(isNull _assignedVehicle) then {\
									[\
										{\
											(CBA_missionTime > 0);\
										},\
										{\
											params ['_vehicle', '_weaponFire', '_waitTime', '_duration', '_framerate'];\
											[_vehicle, parseNumber _duration, parseNumber _framerate, _weaponFire, parseNumber _waitTime] spawn BIS_fnc_unitCapture;\
										},\
										[_assignedVehicle, _weaponFire, _waitTime, _duration, _framerate],\
										30\
									] call CBA_fnc_waitUntilAndExecute;\
								};\
							};\
						";
						defaultValue = "[false, '', false, '', '', '']";
					};
				};
			};
			class KH_Diagnostics
			{
				displayName = "KH Diagnostics";
				collapsed = 1;
				class Attributes
				{
					class KH_DiagnosticsSubcategory
					{
						description = "Displays diagnostic information of the server and all clients to all curators assigned by the <Set Curators> attribute, or the <KH_fnc_setCurators> function.";
						data = "AttributeSystemSubcategory";
						control = "KH_SubcategoryNoHeader1";
					};
					class KH_Diagnostics 
					{
						property = "KH_Diagnostics";
						control = "KH_Diagnostics";
						expression = 
						"\
							_value params ['_toggle', '_3dMode', '_ownership', '_scheduler'];\
							if (_toggle && !is3DEN && isServer) then {\
								KH_var_postInitExecutions pushBack [\
									[true, _3dMode, _ownership, _scheduler],\
									{\
										_this call KH_fnc_diagnostics;\
									}\
								];\
							};\
						";
						defaultValue = "[false, true, false, false]";
					};
				};
			};
			class KH_Dynamic2DAudio
			{
				displayName = "KH Dynamic 2D Audio";
				collapsed = 1;
				class Attributes
				{
					class KH_Dynamic2DAudioSubcategory
					{
						description = "Plays 2D audio for all players at a set interval and chance, under set conditions.";
						data = "AttributeSystemSubcategory";
						control = "KH_SubcategoryNoHeader1";
					};
					class KH_Dynamic2DAudio 
					{
						property = "KH_Dynamic2DAudio";
						control = "KH_Dynamic2DAudio";
						expression = 
						"\
							_value params ['_toggle', '_audio', '_condition', '_interval', '_chance'];\
							if (_toggle && !is3DEN && isServer) then {\
								[parseSimpleArray _audio, compile _condition, parseNumber _interval, _chance] call KH_fnc_dynamic2dAudio;\
							};\
						";
						defaultValue = "[false, '[]', '', '', 1, '100%']";
					};
				};
			};
			class KH_DynamicDisguise
			{
				displayName = "KH Dynamic Disguise";
				collapsed = 1;
				class Attributes
				{
					class KH_DynamicDisguiseSubcategory
					{
						description = "Activates a disguise system that dictates the side affiliation of players based on the uniform and headgear they are wearing. Will work in conjunction with the Dynamic Disguise modules.";
						data = "AttributeSystemSubcategory";
						control = "KH_SubcategoryNoHeader2";
					};
					class KH_DynamicDisguise 
					{
						property = "KH_DynamicDisguise";
						control = "KH_DynamicDisguise";
						expression = 
						"\
							_value params ['_toggle', '_bluforUniforms', '_bluforVests', '_bluforHeadgear', '_opforUniforms', '_opforVests', '_opforHeadgear', '_greenforUniforms', '_greenforVests', '_greenforHeadgear', '_setCaptive'];\
							if (_toggle && !is3DEN && isServer) then {\
								[true, [parseSimpleArray _bluforUniforms, parseSimpleArray _opforUniforms, parseSimpleArray _greenforUniforms], [parseSimpleArray _bluforVests, parseSimpleArray _opforVests, parseSimpleArray _greenforVests], [parseSimpleArray _bluforHeadgear, parseSimpleArray _opforHeadgear, parseSimpleArray _greenforHeadgear], _setCaptive] call KH_fnc_dynamicDisguise;\
							};\
						";
						defaultValue = "[false, '[]', '[]', '[]', '[]', '[]', '[]', '[]', '[]', '[]', false]";
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
						description = "Maintain persistency using an identifier saved in <missionProfileNamespace>. States are saved when the <KH_fnc_endMission> function, or <End Mission> module, is executed, and loaded when valid states are available.";
						data = "AttributeSystemSubcategory";
						control = "KH_SubcategoryNoHeader3";
					};
					class KH_Persistency 
					{
						property = "KH_Persistency";
						control = "KH_Persistency";
						expression = 
						"\
							_value params ['_toggle', '_identifier', '_players', '_playerRespawnType', '_objects', '_world', '_variables'];\
							if (_toggle && !is3DEN && isServer && (_identifier != '')) then {\
								private _parsedVariables = parseSimpleArray _variables;\
								if (_world || (_parsedVariables isNotEqualTo [])) then {\
									[_identifier, _world, _parsedVariables] call KH_fnc_loadMissionState;\
								};\
								if _players then {\
									private _respawnType = 'NONE';\
									switch true do {\
										case (_playerRespawnType == 1): {\
											_respawnType = 'SAVED';\
										};\
										case (_playerRespawnType == 2): {\
											_respawnType = 'INITIAL';\
										};\
										case (_playerRespawnType == 3): {\
											_respawnType = 'DEATH';\
										};\
									};\
									[_identifier, _respawnType] call KH_fnc_loadPlayerLoadouts;\
								};\
								if _objects then {\
									[_identifier] call KH_fnc_loadCargoInventories;\
								};\
								[\
									'CBA',\
									'KH_eve_missionEnded',\
									[_identifier, _players, _objects, _world, _parsedVariables],\
									{\
										_args params ['_identifier', '_players', '_objects', '_world', '_parsedVariables'];\
										[_identifier, _parsedVariables] call KH_fnc_saveMissionState;\
										[_identifier, KH_var_allPlayerUnits] call KH_fnc_savePlayerLoadouts;\
										[_identifier, entities [[], ['Man', 'Logic', 'Animal'], false, false]] call KH_fnc_saveCargoInventories;\
										[_eventName, _localId] call CBA_fnc_removeEventHandler;\
									}\
								] call KH_fnc_addEventHandler;\
							};\
						";
						defaultValue = "[false, profileName, true, 2, true, true, '[]']";
					};
				};
			};
			class KH_SetCurators
			{
				displayName = "KH Set Curators";
				collapsed = 1;
				class Attributes
				{
					class KH_SetCuratorsSubcategory
					{
						description = "Automatically sets curators based on index pair equivalents of Steam IDs and curator modules.";
						data = "AttributeSystemSubcategory";
						control = "KH_SubcategoryNoHeader1";
					};
					class KH_SetCurators
					{
						property = "KH_SetCurators";
						control = "KH_SetCurators";
						expression = 
						"\
							_value params ['_toggle', '_curators', '_curatorModules', '_hide', '_disableDamage'];\
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
						defaultValue = "[false, '[]', '[]', true, true]";
					};
				};
			};
			class KH_SetSideRelations
			{
				displayName = "KH Set Side Relations";
				collapsed = 1;
				class Attributes
				{
					class KH_SetSideRelationsSubcategory
					{
						description = "Set the relationship state of the sides, with <true> designating friendly, and <false> designating hostile.";
						data = "AttributeSystemSubcategory";
						control = "KH_SubcategoryNoHeader2";
					};
					class KH_SetSideRelations
					{
						property = "KH_SetSideRelations";
						control = "KH_SetSideRelations";
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
			class KH_RecoverDisconnectedPlayers
			{
				displayName = "KH Recover Disconnected Players";
				collapsed = 1;
				class Attributes
				{
					class KH_RecoverDisconnectedPlayersSubcategory
					{
						description = "Recovers the unit attributes of players who reconnected after disconnecting.";
						data = "AttributeSystemSubcategory";
						control = "KH_SubcategoryNoHeader1";
					};
					class KH_RecoverDisconnectedPlayers
					{
						property = "KH_RecoverDisconnectedPlayers";
						control = "KH_RecoverDisconnectedPlayers";
						expression = 
						"\
							_value params ['_toggle', '_group', '_loadout', '_transforms', '_vehicle'];\
							if (_toggle && !is3DEN && isServer) then {\
								[true, _group, _loadout, _transforms, _vehicle] call KH_fnc_recoverDisconnectedPlayers;\
							};\
						";
						defaultValue = "[false, true, true, true, true]";
					};
				};
			};
			class KH_RespawnSpectator
			{
				displayName = "KH Respawn Spectator";
				collapsed = 1;
				class Attributes
				{
					class KH_RespawnSpectatorSubcategory
					{
						description = "Makes players who respawn enter spectator mode, which can be terminated if they press the <ESC> key.";
						data = "AttributeSystemSubcategory";
						control = "KH_SubcategoryNoHeader2";
					};
					class KH_RespawnSpectator
					{
						property = "KH_RespawnSpectator";
						control = "KH_RespawnSpectator";
						expression = 
						"\
							_value params ['_toggle', '_type', '_freeCamera', '_playerCamera'];\
							if (_toggle && !is3DEN && isServer) then {\
								private _ace = false;\
								if (_type == 1) then {\
									_ace = true;\
								};\
								[true, _ace, _freeCamera, _playerCamera] call KH_fnc_respawnSpectator;\
							};\
						";
						defaultValue = "[false, 0, false, true]";
					};
				};
			};
			class KH_UnconsciousSpectator
			{
				displayName = "KH Unconscious Spectator";
				collapsed = 1;
				class Attributes
				{
					class KH_UnconsciousSpectatorSubcategory
					{
						description = "Makes players who remain unconscious for a set period of time enter spectator mode, which can be terminated if they press the <ESC> key, or once they become conscious again.";
						data = "AttributeSystemSubcategory";
						control = "KH_SubcategoryNoHeader2";
					};
					class KH_UnconsciousSpectator
					{
						property = "KH_UnconsciousSpectator";
						control = "KH_UnconsciousSpectator";
						expression = 
						"\
							_value params ['_toggle', '_type', '_freeCamera', '_playerCamera', '_delay'];\
							if (_toggle && !is3DEN && isServer) then {\
								private _ace = false;\
								if (_type == 1) then {\
									_ace = true;\
								};\
								[true, _ace, _freeCamera, _playerCamera, parseNumber _delay] call KH_fnc_unconsciousSpectator;\
							};\
						";
						defaultValue = "[false, 0, false, true, '30']";
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
								enableEnvironment [_ambientLife, _ambientSounds, _windCoefficient];\
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
						tooltip = "Limits the maximum view distance to the set amount for all players. Will override custom view distance mods as well. Leave empty for no view distance limit.";
						property = "KH_LimitViewDistance";
						control = "Edit";
						expression = 
						"\
							if ((_value != '') && !is3DEN && isServer) then {\
								KH_var_postInitExecutions pushBack [\
									[true, parseNumber _value],\
									{\
										_this call KH_fnc_limitViewDistance;\
									}\
								];\
							};\
						";
						defaultValue = "''";
					};
				};
			};
		};
	};
};