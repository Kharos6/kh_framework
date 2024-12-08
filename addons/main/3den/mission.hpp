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
					class KH_ServerPreInit
					{
						displayName = "Server Pre Init";
						tooltip = "Unscheduled code to execute on the server before the mission starts. Players and their units are not yet initialized or available, and remote execution will only work if <KH_fnc_execute> is used with <'JIP'>.";
						property = "KH_ServerPreInit";
						control = "EditMulti5";
						expression = 
						"\
							if ((_value != '') && !is3DEN && isServer) then {\
								KH_fnc_serverPreInit = compile _value;\
							}\
							else {\
								KH_fnc_serverPreInit = {};\
							};\
						";
						defaultValue = "''";
					};
					class KH_ServerPostInit
					{
						displayName = "Server Post Init";
						tooltip = "Unscheduled code to execute on the server after the mission starts. Players are initialized, but remote execution is not advised if it requires player units, as they may not yet be available. JIP handling in executions of <KH_fnc_execute> with <'JIP'> during this phase is delayed until the mission init phase begins.";
						property = "KH_ServerPostInit";
						control = "EditMulti5";
						expression = 
						"\
							if ((_value != '') && !is3DEN && isServer) then {\
								KH_fnc_serverPostInit = compile _value;\
							}\
							else {\
								KH_fnc_serverPostInit = {};\
							};\
						";
						defaultValue = "''";
					};		
					class KH_ServerMissionInit
					{
						displayName = "Server Mission Init";
						tooltip = "Unscheduled code to execute on the server after the mission starts, and once all player units have become available. All forms of execution are viable. In the event of a bugged player, this phase can be suspended for a maximum of 60 seconds before it automatically triggers.";
						property = "KH_ServerMissionInit";
						control = "EditMulti5";
						expression = 
						"\
							if ((_value != '') && !is3DEN && isServer) then {\
								KH_fnc_serverMissionInit = compile _value;\
							}\
							else {\
								KH_fnc_serverMissionInit = {};\
							};\
						";
						defaultValue = "''";
					};
					class KH_ServerEndInit
					{
						displayName = "Server End Init";
						tooltip = "Unscheduled code to execute on the server when <KH_fnc_endMission> is executed.";
						property = "KH_ServerEndInit";
						control = "EditMulti5";
						expression = 
						"\
							if ((_value != '') && !is3DEN && isServer) then {\
								KH_fnc_serverEndInit = compile _value;\
							}\
							else {\
								KH_fnc_serverEndInit = {};\
							};\
						";
						defaultValue = "''";
					};
					class KH_HeadlessPreInit
					{
						displayName = "Headless Pre Init";
						tooltip = "Unscheduled code to execute on headless clients before the mission starts. Players and their units may not yet be initialized or available, and remote execution may only work if <KH_fnc_execute> is used with <'JIP'>.";
						property = "KH_HeadlessPreInit";
						control = "EditMulti5";
						expression = 
						"\
							if ((_value != '') && !is3DEN && isServer) then {\
								KH_fnc_headlessPreInit = compile _value;\
							}\
							else {\
								KH_fnc_headlessPreInit = {};\
							};\
						";
						defaultValue = "''";
					};
					class KH_HeadlessPostInit
					{
						displayName = "Headless Post Init";
						tooltip = "Unscheduled code to execute on headless clients after the mission starts. Players are initialized, but remote execution is not advised if it requires player units, as they may not yet be available. JIP handling in executions of <KH_fnc_execute> with <'JIP'> during this phase is delayed until the mission init phase begins.";
						property = "KH_HeadlessPostInit";
						control = "EditMulti5";
						expression = 
						"\
							if ((_value != '') && !is3DEN && isServer) then {\
								KH_fnc_headlessPostInit = compile _value;\
							}\
							else {\
								KH_fnc_headlessPostInit = {};\
							};\
						";
						defaultValue = "''";
					};		
					class KH_HeadlessMissionInit
					{
						displayName = "Headless Mission Init";
						tooltip = "Unscheduled code to execute on headless clients after the mission starts, and once all player units have become available. All forms of execution are viable. In the event of a bugged player, this phase can be suspended for a maximum of 60 seconds before it automatically triggers.";
						property = "KH_HeadlessMissionInit";
						control = "EditMulti5";
						expression = 
						"\
							if ((_value != '') && !is3DEN && isServer) then {\
								KH_fnc_headlessMissionInit = compile _value;\
							}\
							else {\
								KH_fnc_headlessMissionInit = {};\
							};\
						";
						defaultValue = "''";
					};
					class KH_HeadlessEndInit
					{
						displayName = "Headless End Init";
						tooltip = "Unscheduled code to execute on headless clients when <KH_fnc_endMission> is executed.";
						property = "KH_HeadlessEndInit";
						control = "EditMulti5";
						expression = 
						"\
							if ((_value != '') && !is3DEN && isServer) then {\
								KH_fnc_headlessEndInit = compile _value;\
							}\
							else {\
								KH_fnc_headlessEndInit = {};\
							};\
						";
						defaultValue = "''";
					};
					class KH_PlayerPreInit
					{
						displayName = "Player Pre Init";
						tooltip = "Unscheduled code to execute locally to each player before the mission starts. The local player unit is not yet available.";
						property = "KH_PlayerPreInit";
						control = "EditMulti5";
						expression = 
						"\
							if ((_value != '') && !is3DEN && isServer) then {\
								KH_fnc_playerPreInit = compile _value;\
							}\
							else {\
								KH_fnc_playerPreInit = {};\
							};\
						";
						defaultValue = "''";
					};
					class KH_PlayerPostInit
					{
						displayName = "Player Post Init";
						tooltip = "Unscheduled code to execute locally to each player after the mission starts. The local player unit is available, and this is the ideal phase to execute code that requires the local player unit. Other players are initialized, but remote execution is not advised if it requires player units, as they may not yet be initialized. JIP handling in executions of <KH_fnc_execute> with <'JIP'> during this phase is delayed until the mission init phase begins.";
						property = "KH_PlayerPostInit";
						control = "EditMulti5";
						expression = 
						"\
							if ((_value != '') && !is3DEN && isServer) then {\
								KH_fnc_playerPostInit = compile _value;\
							}\
							else {\
								KH_fnc_playerPostInit = {};\
							};\
						";
						defaultValue = "''";
					};
					class KH_PlayerMissionInit
					{
						displayName = "Player Mission Init";
						tooltip = "Unscheduled code to execute locally to each player after the mission starts, and once all other player units have become available. All forms of execution are viable. In the event of a bugged player, this phase can be suspended for a maximum of 60 seconds before it automatically triggers.";
						property = "KH_PlayerMissionInit";
						control = "EditMulti5";
						expression = 
						"\
							if ((_value != '') && !is3DEN && isServer) then {\
								KH_fnc_playerMissionInit = compile _value;\
							}\
							else {\
								KH_fnc_playerMissionInit = {};\
							};\
						";
						defaultValue = "''";
					};
					class KH_PlayerRespawnInit
					{
						displayName = "Player Respawn Init";
						tooltip = "Unscheduled code to execute locally to each player after they respawn. Local player unit is now the respawned unit. Passed arguments available through <_this> are: [_corpse].";
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
						displayName = "Player Switch Init";
						tooltip = "Unscheduled code to execute locally to each player after they switch to another unit. Local player unit is now the switched unit. Passed arguments available through <_this> are: [_previousUnit].";
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
					class KH_PlayerJIPInit
					{
						displayName = "Player JIP Init";
						tooltip = "Unscheduled code to execute locally to each player that joins after all of the initial player units have become available.";
						property = "KH_PlayerJIPInit";
						control = "EditMulti5";
						expression = 
						"\
							if ((_value != '') && !is3DEN && isServer) then {\
								KH_fnc_playerJipInit = compile _value;\
							}\
							else {\
								KH_fnc_playerJipInit = {};\
							};\
						";
						defaultValue = "''";
					};
					class KH_PlayerEndInit
					{
						displayName = "Player End Init";
						tooltip = "Unscheduled code to execute locally to each player before the mission ends when <KH_fnc_endMission> is executed.";
						property = "KH_PlayerEndInit";
						control = "EditMulti5";
						expression = 
						"\
							if ((_value != '') && !is3DEN && isServer) then {\
								KH_fnc_playerEndInit = compile _value;\
							}\
							else {\
								KH_fnc_playerEndInit = {};\
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
					class KH_FixUnconsciousAnimations
					{
						displayName = "Fix Unconscious Animations";
						tooltip = "Corrects the animation state of players who are registered as conscious, but remain stuck in an unconscious animation.";
						property = "KH_FixUnconsciousAnimations";
						control = "Checkbox";
						expression = 
						"\
							if (_value && !is3DEN && isServer) then {\
								[true] call KH_fnc_fixUnconsciousAnimations;\
							};\
						";
						defaultValue = "false";
					};
					class KH_PublicFunctions
					{
						displayName = "Public Functions";
						tooltip = "Set one or more mission functions, in form of strings of their name and file, to be made available to all clients, in format: [['name1', 'file1'], ['name2', 'file2'], ['name3', 'file3']].";
						property = "KH_PublicFunctions";
						control = "EditMulti5";
						expression = 
						"\
							if ((_value != '') && (_value != '[]') && !is3DEN && isServer) then {\
								private _functions = createHashMapFromArray (parseSimpleArray _value);\
								{\
									uiNamespace setVariable [_x, (compile (preprocessFileLineNumbers _y)), true];\
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
										titleText [_text, 'BLACK IN', 999];\
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
						description = "Plays a sequence of camera shots for players present at the start of the mission, and optionally JIP players. The attributes of each camera are interpolated based on index value equivalents from the different arrays of attributes. For example, the third element in <Position Entities> will take effect on the same shot as the third element in <Target Entities>.";
						data = "AttributeSystemSubcategory";
						control = "KH_SubcategoryNoHeader4";
					};
					class KH_CameraSequence 
					{
						property = "KH_CameraSequence";
						control = "KH_CameraSequence";
						expression = 
						"\
							_value params ['_toggle', '_positionEntities', '_targetEntities', '_fovs', '_commitTimes', '_durations', '_visionTypes', '_cinematicBorders', '_disableUserInput', '_jip'];\
							if (_toggle && !is3DEN && isServer) then {\
								private _parsedPositionEntities = [];\
								private _parsedTargetEntities = [];\
								{\
									_parsedPositionEntities pushBack (missionNamespace getVariable [_x, objNull]);\
								} forEach _positionEntities;\
								{\
									_parsedTargetEntities pushBack (missionNamespace getVariable [_x, objNull]);\
								} forEach _targetEntities;\
								[\
									['CBA'],\
									'KH_eve_playerLoaded',\
									[_parsedPositionEntities, _parsedTargetEntities, parseSimpleArray _fovs, parseSimpleArray _commitTimes, parseSimpleArray _durations, parseSimpleArray _visionTypes, _cinematicBorders, _disableUserInput, _jip],\
									{\
										params ['_unit'];\
										_args params ['_positionEntities', '_targetEntities', '_fovs', '_commitTimes', '_durations', '_visionTypes', '_cinematicBorders', '_disableUserInput', '_jip'];\
										if _jip then {\
											[[_positionEntities, _targetEntities, _fovs, _commitTimes, _durations, _visionTypes, _cinematicBorders, _disableUserInput], 'KH_fnc_cameraSequence', _unit, 'THIS_FRAME'] call KH_fnc_execute;\
										}\
										else {\
											if ((_unit in KH_var_initialPlayerUnits) && !KH_var_playersInitialized || (CBA_missionTime < 60)) then {\
												[[_positionEntities, _targetEntities, _fovs, _commitTimes, _durations, _visionTypes, _cinematicBorders, _disableUserInput], 'KH_fnc_cameraSequence', _unit, 'THIS_FRAME'] call KH_fnc_execute;\
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
									[_assignedVehicle, parseNumber _duration, parseNumber _framerate, _weaponFire, parseNumber _waitTime] spawn BIS_fnc_unitCapture;\
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
						description = "Displays diagnostic information of the server and all clients to the current admin.";
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
								[\
									{\
										(time > 0);
									},\
									{\
										params ['_3dMode', '_ownership', '_scheduler'];\
										[true, _3dMode, _ownership, _scheduler] call KH_fnc_diagnostics;\
									},\
									[_3dMode, _ownership, _scheduler],\
									30\
								] call CBA_fnc_waitUntilAndExecute;\
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
						defaultValue = "[false, '[]', 'true', '', 1, '100%']";
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
						description = "Activates a disguise system that dictates the side affiliation of players based on the uniform and headgear they are wearing.";
						data = "AttributeSystemSubcategory";
						control = "KH_SubcategoryNoHeader2";
					};
					class KH_DynamicDisguise 
					{
						property = "KH_DynamicDisguise";
						control = "KH_DynamicDisguise";
						expression = 
						"\
							_value params ['_toggle', '_bluforUniforms', '_bluforHeadgear', '_redforUniforms', '_redforHeadgear', '_greenforUniforms', '_greenforHeadgear', '_setCaptive'];\
							if (_toggle && !is3DEN && isServer) then {\
								[true, [parseSimpleArray _bluforUniforms, parseSimpleArray _redforUniforms, parseSimpleArray _greenforUniforms], [parseSimpleArray _bluforHeadgear, parseSimpleArray _redforHeadgear, parseSimpleArray _greenforHeadgear], _setCaptive] call KH_fnc_dynamicDisguise;\
							};\
						";
						defaultValue = "[false, '[]', '[]', '[]', '[]', '[]', '[]', false]";
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
						description = "Maintain persistency using an identifier saved in <missionProfileNamespace>. States are saved when <KH_fnc_endMission> is executed, and loaded when valid states are available.";
						data = "AttributeSystemSubcategory";
						control = "KH_SubcategoryNoHeader2";
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
								if (_world || (_variables isNotEqualTo [])) then {\
									[_identifier, _world, _parsedVariables] call KH_fnc_loadMissionState;\
								};\
								if _players then {\
									[_identifier, _playerRespawnType] call KH_fnc_loadPlayerLoadouts;\
								};\
								if _objects then {\
									[_identifier] call KH_fnc_loadCargoInventories;\
								};\
								[\
									['CBA'],\
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
								[true, parseNumber _value] call KH_fnc_limitViewDistance;\
							};\
						";
						defaultValue = "''";
					};
				};
			};
		};
	};
};