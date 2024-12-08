class Object
{
	class AttributeCategories
	{
		class KH_ConditionalPresence
		{
			displayName = "KH Conditional Presence";
			collapsed = 1;
			class Attributes
			{
				class KH_ConditionalPresenceSubcategory
				{
					description = "Specify a variable which, every time it is broadcasted to the server with <publicVariableServer> or <publicVariableClient>, changes the simulation and visibility of this entity. If <true>, the entity will become simulated and visible. If <false>, the entity will become unsimulated and invisible.";
					data = "AttributeSystemSubcategory";
					control = "KH_SubcategoryNoHeader3";
				};
				class KH_ConditionalPresence 
				{
					property = "KH_ConditionalPresence";
					control = "KH_ConditionalPresence";
					expression = 
					"\
						_value params ['_toggle', '_variableName', '_initialization', '_invert'];\
						if (_toggle && !is3DEN && (_variableName != '')) then {\
							[_this, _variableName, compile _initialization, _invert] call KH_fnc_conditionalPresence;\
						};\
					";
					defaultValue = "[false, '', '', false]";
				};
			};
		};
		class KH_ConvertToAgent
		{
			displayName = "KH Convert To Agent";
			collapsed = 1;
			class Attributes
			{
				class KH_ConvertToAgentSubcategory
				{
					description = "Converts this unit to an agent upon mission start. Most Eden attributes will be lost, but can be replicated using the Initialization field.";
					data = "AttributeSystemSubcategory";
					control = "KH_SubcategoryNoHeader2";
				};
				class KH_ConvertToAgent 
				{
					property = "KH_ConvertToAgent";
					control = "KH_ConvertToAgent";
					expression = 
					"\
						_value params ['_toggle', '_initialization'];\
						if (_toggle && !is3DEN) then {\
							[[_this], compile _initialization] call KH_fnc_convertToAgent;\
						};\
					";
					defaultValue = "[false, '']";
					condition = "objectControllable";
				};
			};
		};
		class KH_Dynamic3DAudio
		{
			displayName = "KH Dynamic 3D Audio";
			collapsed = 1;
			class Attributes
			{
				class KH_Dynamic3DAudioSubcategory
				{
					description = "Plays 3D audio emitting from this entity for all players at a set interval and chance, under set conditions.";
					data = "AttributeSystemSubcategory";
					control = "KH_SubcategoryNoHeader2";
				};
				class KH_Dynamic3DAudio 
				{
					property = "KH_Dynamic3DAudio";
					control = "KH_Dynamic3DAudio";
					expression = 
					"\
						_value params ['_toggle', '_audio', '_condition', '_interval', '_chance'];\
						if (_toggle && !is3DEN) then {\
							[[_this], parseSimpleArray _audio, compile _condition, parseNumber _interval, _chance] call KH_fnc_dynamic3dAudio;\
						};\
					";
					defaultValue = "[false, '[]', 'true', '', 1, '100%']";
				};
			};
		};
		class KH_EquipableObject
		{
			displayName = "KH Equipable Object";
			collapsed = 1;
			class Attributes
			{
				class KH_EquipableObjectSubcategory
				{
					description = "Designate this object to be equipable by players, allowing them to carry it on the designated position on their body.";
					data = "AttributeSystemSubcategory";
					control = "KH_SubcategoryNoHeader2";
				};
				class KH_EquipableObject 
				{
					property = "KH_EquipableObject";
					control = "KH_EquipableObject";
					expression = 
					"\
						_value params ['_toggle', '_bone', '_position', '_rotation', '_scale', '_mass', '_hideInVehicles', '_toggleEquip', '_exclusive', '_event', '_objectName'];\
						if (_toggle && !is3DEN) then {\
							[objNull, _this, _bone, parseSimpleArray _position, parseSimpleArray _rotation, parseNumber _scale, parseNumber _mass, _hideInVehicles, _toggleEquip, _exclusive, _event, _objectName] call KH_fnc_equipableObject;\
						};\
					";
					defaultValue = "[false, '', '[]', '[]', '', '', true, true, true, '', '']";
					condition = "1 - objectControllable";
				};
			};
		};
		class KH_FultonExtract
		{
			displayName = "KH Fulton Extract";
			collapsed = 1;
			class Attributes
			{
				class KH_FultonExtractSubcategory
				{
					description = "Designate this object as a fulton extraction point, allowing players to interact with it in order to summon a balloon that can be picked up by the desired vehicle, after which anchored players will be roped into said vehicle.";
					data = "AttributeSystemSubcategory";
					control = "KH_SubcategoryNoHeader3";
				};
				class KH_FultonExtract 
				{
					property = "KH_FultonExtract";
					control = "KH_FultonExtract";
					expression = 
					"\
						_value params ['_toggle', '_vehicles', '_height', '_distance', '_maximumParticipants', '_duration', '_objectName'];\
						if (_toggle && !is3DEN) then {\
							[\
								{\
									(CBA_missionTime > 0);\
								},\
								{\
									params ['_entity', '_vehicles', '_height', '_distance', '_maximumParticipants', '_duration', '_objectName'];\
									private _parsedVehicles = [];\
									{\
										_parsedVehicles pushBack (missionNamespace getVariable [_x, objNull]);\
									} forEach (parseSimpleArray _vehicles);\
									[_entity, _parsedVehicles, _height, parseNumber _distance, parseNumber _maximumParticipants, parseNumber _duration, _objectName] call KH_fnc_fultonExtract;\
								},\
								[_this, _vehicles, _height, _distance, _maximumParticipants, _duration, _objectName]\
							] call CBA_fnc_waitUntilAndExecute;\
						};\
					";
					defaultValue = "[false, '[]', 100, '', '', '', '', '100m']";
					condition = "1 - objectControllable";
				};
			};
		};
		class KH_SetAnimation
		{
			displayName = "KH Set Animation";
			collapsed = 1;
			class Attributes
			{
				class KH_SetAnimationSubcategory
				{
					description = "Plays the desired animation on this unit.";
					data = "AttributeSystemSubcategory";
					control = "KH_SubcategoryNoHeader1";
				};
				class KH_SetAnimation 
				{
					property = "KH_SetAnimation";
					control = "KH_SetAnimation";
					expression = 
					"\
						_value params ['_toggle', '_animation', '_duration', '_interruptable'];\
						if (_toggle && !is3DEN) then {\
							private _interruptType = 'NONE';\
							if _interruptable then {\
								_interruptType = 'GROUP';\
							};\
							[_this, [_animation], _this, _this, _this, _this, _this, parseNumber _duration, false, false, false, false, _interruptType, ''] call KH_fnc_setAnimation;\
						};\
					";
					defaultValue = "[false, '', '', false]";
					condition = "objectControllable";
				};
			};
		};
		class KH_SetCameraTexture
		{
			displayName = "KH Set Camera Texture";
			collapsed = 1;
			class Attributes
			{
				class KH_SetCameraTextureSubcategory
				{
					description = "Displays the view of a generated camera onto a texture on this entity.";
					data = "AttributeSystemSubcategory";
					control = "KH_SubcategoryNoHeader1";
				};
				class KH_SetCameraTexture 
				{
					property = "KH_SetCameraTexture";
					control = "KH_SetCameraTexture";
					expression = 
					"\
						_value params ['_toggle', '_positionEntity', '_targetEntity', '_texture', '_renderTarget'];\
						if (_toggle && !is3DEN) then {\
							[\
								{\
									(CBA_missionTime > 0);\
								},\
								{\
									params ['_entity', '_positionEntity', '_targetEntity', '_texture', '_renderTarget'];\
									[[_entity, missionNamespace getVariable [_positionEntity, objNull], missionNamespace getVariable [_targetEntity, objNull], parseNumber _texture, _renderTarget + 1], 'KH_fnc_setCameraTexture', ['JIP', 'PLAYERS', _entity, true, false], 'THIS_FRAME'] call KH_fnc_execute;\
								},\
								[_this, _positionEntity, _targetEntity, _texture, _renderTarget]\
							] call CBA_fnc_waitUntilAndExecute;\
						};\
					";
					defaultValue = "[false, '', '', '', 0]";
				};
			};
		};
		class KH_SetVideoTexture
		{
			displayName = "KH Set Video Texture";
			collapsed = 1;
			class Attributes
			{
				class KH_SetVideoTextureSubcategory
				{
					description = "Plays a video, and optionally audio, on the selected texture on this entity, at a desired interval, while the entity remains alive.";
					data = "AttributeSystemSubcategory";
					control = "KH_SubcategoryNoHeader2";
				};
				class KH_SetVideoTexture 
				{
					property = "KH_SetVideoTexture";
					control = "KH_SetVideoTexture";
					expression = 
					"\
						_value params ['_toggle', '_video', '_texture', '_audio', '_interval'];\
						if (_toggle && !is3DEN) then {
							[\
								[_entity, _video, parseNumber _texture, _audio, parseNumber _interval],\
								{\
									params ['_entity', '_video', '_texture', '_audio', '_interval'];\
									[\
										{\
											params ['_entity', '_video', '_texture', '_audio'];\
											if (alive _entity) then {\
												[_entity, _video, _texture, _audio] call KH_fnc_setVideoTexture;\
											}\
											else {\
												_entity say3D '';\
												object setObjectTexture [_texture, '#(rgb,8,8,3)color(0,0,0,1)'];\
												[_handle] call CBA_fnc_removePerFrameHandler;\
											};\
										},\
										_interval,\
										[_entity, _video, _texture, _audio]\
									] call CBA_fnc_addPerFrameHandler;\
								},\
								['JIP', 'PLAYERS', _entity, true, false],\
								'THIS_FRAME'\
							] call KH_fnc_execute;\
						};\
					";
					defaultValue = "[false, '', '', '', '']";
				};
			};
		};
		class KH_Teleporter
		{
			displayName = "KH Teleporter";
			collapsed = 1;
			class Attributes
			{
				class KH_TeleporterSubcategory
				{
					description = "Designates this entity as a teleporter, adding an action available to all players that allows them to teleport to the desired position.";
					data = "AttributeSystemSubcategory";
					control = "KH_SubcategoryNoHeader2";
				};
				class KH_Teleporter 
				{
					property = "KH_Teleporter";
					control = "KH_Teleporter";
					expression = 
					"\
						_value params ['_toggle', '_position', '_rotation', '_transition', '_heal', '_freefallHeight', '_initialization', '_name'];\
						if (_toggle && !is3DEN) then {\
							[\
								{\
									(CBA_missionTime > 0);\
								},\
								{\
									params ['_entity', '_position', '_rotation', '_transition', '_heal', '_freefallHeight', '_initialization', '_name'];\
									[\
										[_entity, missionNamespace getVariable [_position, objNull], missionNamespace getVariable [_rotation, objNull], parseNumber _transition, _heal, parseNumber _freefallHeight, compile _initialization, _name],\
										{\
											params ['_entity', '_position', '_rotation', '_transition', '_heal', '_freefallHeight', '_initialization', '_name'];\
											[\
												_object,\
												_name,\
												'\a3\data_f_destroyer\data\UI\IGUI\Cfg\holdactions\holdAction_loadVehicle_ca.paa',\
												'\a3\data_f_destroyer\data\UI\IGUI\Cfg\holdactions\holdAction_loadVehicle_ca.paa',\
												'((_this distance _target) < 4)',\
												'((_caller distance _target) < 4)',\
												{},\
												{},\
												{\
													(_this select 3) params ['_entity', '_position', '_rotation', '_transition', '_heal', '_freefallHeight', '_initialization'];\
													[[player], _position, _rotation, false, _transition, _heal, _freefallHeight, _initialization] call KH_fnc_teleport;\
												},\
												{},\
												[_entity, _position, _rotation, _transition, _heal, _freefallHeight, _initialization],\
												1,\
												0,\
												false,\
												false,\
												true\
											] call BIS_fnc_holdActionAdd;\
										},\
										['JIP', 'PLAYERS', _entity, false, false],\
										'THIS_FRAME'\
									] call KH_fnc_execute;\
								},\
								[_this, _position, _rotation, _transition, _heal, _freefallHeight, _initialization, _name]\
							] call CBA_fnc_waitUntilAndExecute;\
						};\
					";
					defaultValue = "[false, '', '', '', false, '', '', '']";
				};
			};
		};
		class KH_TransferToHeadlessClient
		{
			displayName = "KH Transfer To Headless Client";
			collapsed = 1;
			class Attributes
			{
				class KH_TransferToHeadlessClientSubcategory
				{
					description = "Designate this entity for transfer to a headless client.";
					data = "AttributeSystemSubcategory";
					control = "KH_SubcategoryNoHeader1";
				};
				class KH_TransferToHeadlessClient 
				{
					property = "KH_TransferToHeadlessClient";
					control = "KH_TransferToHeadlessClient";
					expression = 
					"\
						_value params ['_toggle', '_owner', '_recreate', '_initialization'];\
						if (_toggle && !is3DEN) then {\
							KH_var_headlessClientTransfers pushBack [_this, _owner, _recreate];\
							if (_initialization != '') then {\
								[\
									{\
										params ['_unit', '_owner'];\
										((owner _unit) == (owner _owner));\
									},\
									{\
										private _unit = _this select 0;\
										private _initialization = _this select 2;\
										[_unit] call _initialization;\
									},\
									[_this, _owner, compile _initialization],\
									30\
								] call CBA_fnc_waitUntilAndExecute;\
							};\
						};\
					";
					defaultValue = "[false, '', false, '']";
				};
			};
		};
		class KH_VehicleSequence
		{
			displayName = "KH Vehicle Sequence";
			collapsed = 1;
			class Attributes
			{
				class KH_VehicleSequenceSubcategory
				{
					description = "Play a vehicle sequence on this entity when the mission starts.";
					data = "AttributeSystemSubcategory";
					control = "KH_SubcategoryNoHeader1";
				};
				class KH_VehicleSequence 
				{
					property = "KH_VehicleSequence";
					control = "KH_VehicleSequence";
					expression = 
					"\
						_value params ['_toggle', '_movementData', '_firingData', '_disableDamage', '_endPosition'];\
						if (_toggle && !is3DEN) then {\
							[_this, _movementData, _firingData, _disableDamage, missionNamespace getVariable [_endPosition, objNull]] call KH_fnc_vehicleSequence;\
						};\
					";
					defaultValue = "[false, '', '', false, '']";
					condition = "objectVehicle";
				};
			};
		};
		class KH_VisualTrigger
		{
			displayName = "KH Visual Trigger";
			collapsed = 1;
			class Attributes
			{
				class KH_VisualTriggerSubcategory
				{
					description = "Designate this entity as a trigger that activates and deactivates based on the entity's presence on a player's screen. This trigger does not activate for curators or spectators.";
					data = "AttributeSystemSubcategory";
					control = "KH_SubcategoryNoHeader2";
				};
				class KH_VisualTrigger 
				{
					property = "KH_VisualTrigger";
					control = "KH_VisualTrigger";
					expression = 
					"\
						_value params ['_toggle', '_screenMultiplier', '_proximity', '_distance', '_conditionServer', '_conditionPlayer', '_trueFunction', '_falseFunction', '_repeatable', '_interval', '_shared'];\
						if (_toggle && !is3DEN) then {\
							[_this, _screenMultiplier, parseNumber _proximity, parseNumber _distance, compile _conditionServer, compile _conditionPlayer, compile _trueFunction, compile _falseFunction, _repeatable, parseNumber _interval, _shared] call KH_fnc_visualTrigger;\
						};\
					";
					defaultValue = "[false, 1, '', '', 'true', 'true', '', '', false, '', true, '100%']";
				};
			};
		};
		class KH_Miscellaneous
		{
			displayName = "KH Miscellaneous";
			collapsed = 1;
			class Attributes
			{
				class KH_AnimationSpeed
				{
					displayName = "Animation Speed";
					tooltip = "Sets the animation speed of this unit. Leave empty for no change.";
					property = "KH_AnimationSpeed";
					control = "Edit";
					expression = 
					"\
						if ((_value != '') && !is3DEN) then {\
							[\
								[_this, parseNumber _value],\
								{\
									params ['_unit', '_speed'];\
									_unit setAnimSpeedCoef _speed;\
								},\
								['JIP', 'PLAYERS', _this, false, false],\
								'THIS_FRAME'\
							] call KH_fnc_execute;\
						};\
					";
					defaultValue = "''";
					condition = "objectControllable";
				};
				class KH_ArrayBuilder
				{
					displayName = "Array Builder";
					tooltip = "Specify an array of one or more strings of global variables that will be made into an array, made public, and contain this entity and any other entities utilizing this function. In format ['globalVariable1', 'globalVariable2', 'globalVariable3']";
					property = "KH_ArrayBuilder";
					control = "Edit";
					expression = 
					"\
						if ((_value != '') && (_value != '[]') && !is3DEN) then {\
							{\
								private _originalValueArray = missionNamespace getVariable [_x, []];\
								_originalValueArray pushBack _this;\
								missionNamespace setVariable [_x, _originalValueArray];\
								private _originalArrayArray = missionNamespace getVariable ['KH_var_entityArrayBuilderArrays', []];\
								_originalArrayArray pushBackUnique _x;\
								missionNamespace setVariable ['KH_var_entityArrayBuilderArrays', _originalArrayArray];\
							} forEach (parseSimpleArray _value);\
						};\
					";
					defaultValue = "'[]'";
				};
				class KH_DynamicSimulation
				{
					displayName = "Dynamic Simulation";
					tooltip = "Checks the targeted entity's distance to each player, and toggles its simulation based on the set distance, turning it on if a player is within it, and turning it off is a player is outside of it. Leave empty to disable it. This function is intended to allow additional distance flexibility alongside the standard Dynamic Simulation system.";
					property = "KH_DynamicSimulation";
					control = "Edit";
					expression = 
					"\
						if ((_value != '') && !is3DEN) then {\
							[[_this], parseNumber _value] call KH_fnc_dynamicSimulation;\
						};\
					";
					defaultValue = "''";
				};
				class KH_LockInventory
				{
					displayName = "Lock Inventory";
					tooltip = "<true> locks the inventory of this entity, preventing the inventory from being accessible.";
					property = "KH_LockInventory";
					control = "Checkbox";
					expression = 
					"\
						if (_value && !is3DEN) then {\
							[\
								[_this],\
								{\
									params ['_entity'];\
									_entity lockInventory true;\
								},\
								['JIP', 'PLAYERS', _this, false, false],\
								'THIS_FRAME'\
							] call KH_fnc_execute;\
						};\
					";
					defaultValue = "false";
				};
				class KH_SetRenegade
				{
					displayName = "Set Renegade";
					tooltip = "<true> declares the unit as a renegade, causing units from its own and allied sides to attack it, but it will not attack them back.";
					property = "KH_SetRenegade";
					control = "Checkbox";
					expression = 
					"\
						if (_value && !is3DEN) then {\
							_this addRating -10000;\
						};\
					";
					defaultValue = "false";
					condition = "objectControllable";
				};
			};
		};
	};
};