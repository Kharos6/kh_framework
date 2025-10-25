class Object
{
	class AttributeCategories
	{
		class KH_Miscellaneous
		{
			displayName = "KH Miscellaneous";
			collapsed = 1;
			class Attributes
			{
				class KH_AnimationSpeed
				{
					displayName = "Animation Speed";
					tooltip = "Sets the animation speed of this unit. Set to -1 for no change.";
					property = "KH_AnimationSpeed";
					control = "Edit";
					expression = 
					"\
						if ((_value isNotEqualTo -1) && !is3DEN) then {\
							KH_var_postInitExecutions pushBack [\
								[_this, _value],\
								{\
									[\
										_this,\
										{\
											params ['_unit', '_speed'];\
											_unit setAnimSpeedCoef _speed;\
										},\
										'GLOBAL',\
										true,\
										['JIP', _this select 0, true, '']\
									] call KH_fnc_execute;\
								}\
							];\
						};\
					";
					defaultValue = "-1";
					validate = "number";
					typeName = "NUMBER";
					condition = "objectControllable";
				};
				class KH_ArrayBuilder
				{
					displayName = "Array Builder";
					tooltip = "Specify an array of strings of global variables that will be made into an array, made public, and contain this entity and any other entities utilizing this function. In format 'globalVariable1', 'globalVariable2', 'globalVariable3', ...";
					property = "KH_ArrayBuilder";
					control = "Edit";
					expression = 
					"\
						if ((_value isNotEqualTo '') && !is3DEN) then {\
							private _array = parseSimpleArray (['[', _value, ']'] joinString '');\
							_this setVariable ['KH_var_assignedEntityArrayBuilderArrays', _array];\
							{\
								private _originalValueArray = missionNamespace getVariable [_x, []];\
								_originalValueArray pushBack _this;\
								missionNamespace setVariable [_x, _originalValueArray];\
								private _originalArrayArray = missionNamespace getVariable ['KH_var_entityArrayBuilderArrays', []];\
								_originalArrayArray pushBackUnique _x;\
								missionNamespace setVariable ['KH_var_entityArrayBuilderArrays', _originalArrayArray];\
							} forEach _array;\
						};\
					";
					defaultValue = "''";
				};
				class KH_FreefallHeight
				{
					displayName = "Freefall Height";
					tooltip = "Height above which this unit will enter freefall, in meters. Set to -1 for no change from the default of 100 metres.";
					property = "KH_FreefallHeight";
					control = "Edit";
					expression = 
					"\
						if ((_value isNotEqualTo -1) && !is3DEN) then {\
							_this setUnitFreefallHeight _value;\
							[\
								[_this, _value],\
								{\
									params ['_entity', '_value'];\
									_entity setUnitFreefallHeight _value;\
								},\
								_this,\
								true,\
								[\
									'PERSISTENT',\
									true,\
									[],\
									{},\
									''\
								]\
							] call KH_fnc_execute;\
						};\
					";
					defaultValue = "-1";
					validate = "number";
					typeName = "NUMBER";
					condition = "objectControllable";
				};
				class KH_PlayerObjectInit
				{
					displayName = "Player Object Init";
					tooltip = "Unscheduled code executed locally to the player who loads into the mission as this unit. The local player variable is valid as this unit.";
					property = "KH_PlayerObjectInit";
					control = "EditCodeMulti5";
					expression = 
					"\
						if ((_value isNotEqualTo '') && !is3DEN) then {\
							KH_var_postInitExecutions pushBack [\
								[_this, compile _value],\
								{\
									params ['_entity', '_function'];\
									[\
										'CBA',\
										'KH_eve_playerLoaded',\
										[_entity, _function],\
										{\
											_args params ['_entity', '_function'];\
											if (player isEqualTo _entity) then {\
												call _function;\
											};\
										}\
									] call KH_fnc_addEventHandler;\
								}\
							];\
						};\
					";
					defaultValue = "''";
					validate = "expression";
					condition = "objectControllable";
				};
				class KH_ServerObjectInit
				{
					displayName = "Server Object Init";
					tooltip = "Unscheduled code executed on the server with this entity passed as an argument. Passed arguments available through _this are: [_entity (OBJECT)].";
					property = "KH_ServerObjectInit";
					control = "EditCodeMulti5";
					expression = 
					"\
						if ((_value isNotEqualTo '') && !is3DEN) then {\
							KH_var_postInitExecutions pushBack [\
								[_this, compile _value],\
								{\
									params ['_entity', '_function'];\
									[_entity] call _function;\
								}\
							];\
						};\
					";
					defaultValue = "''";
					validate = "expression";
				};
				class KH_LockInventory
				{
					displayName = "Lock Inventory";
					tooltip = "True locks the inventory of this entity.";
					property = "KH_LockInventory";
					control = "Checkbox";
					expression = 
					"\
						if (_value && !is3DEN) then {\
							KH_var_postInitExecutions pushBack [\
								[_this],\
								{\
									[\
										_this,\
										{\
											params ['_entity'];\
											_entity lockInventory true;\
										},\
										'GLOBAL',\
										true,\
										['JIP', _this select 0, false, '']\
									] call KH_fnc_execute;\
								}\
							];\
						};\
					";
					defaultValue = "false";
				};
				class KH_Renegade
				{
					displayName = "Renegade";
					tooltip = "True declares the unit as a renegade, causing units from all sides to attack it. Ideal for players, as AI on this unit will not attack its own side even if endangered.";
					property = "KH_Renegade";
					control = "Checkbox";
					expression = 
					"\
						if (_value && !is3DEN) then {\
							[\
								[_this],\
								{\
									params ['_entity'];\
									[_entity, false] call KH_fnc_setRating;\
								},\
								_this,\
								true,\
								[\
									'PERSISTENT',\
									true,\
									[],\
									{},\
									''\
								]\
							] call KH_fnc_execute;\
						};\
					";
					defaultValue = "false";
					condition = "objectControllable";
				};
				class KH_SetRandomLoadout
				{
					displayName = "Set Random Loadout";
					tooltip = "Specify either an array of loadouts, or a global variable containing an array of loadouts, that will be selected from at random and applied to this unit.";
					property = "KH_SetRandomLoadout";
					control = "Edit";
					expression = 
					"\
						if ((_value isNotEqualTo '') && (_value isNotEqualTo '[]') && !is3DEN) then {\
							KH_var_postInitExecutions pushBack [\
								[_this, _value],\
								{\
									params ['_entity', '_loadouts'];\
									if (('[' in _loadouts) || (']' in _loadouts)) then {\
										_entity setUnitLoadout (selectRandom (parseSimpleArray _loadouts));\
									}\
									else {\
										_entity setUnitLoadout (selectRandom (missionNamespace getVariable [_loadouts, []]));\
									};\
								}\
							];\
						};\
					";
					defaultValue = "''";
					condition = "objectControllable";
				};
			};
		};
		class KH_ConditionalPresence
		{
			displayName = "KH Conditional Presence";
			collapsed = 1;
			class Attributes
			{
				class KH_ConditionalPresenceSubcategory
				{
					description = "Specify a CBA event which changes the simulation and visibility of this entity every time it is broadcasted to the server. If its argument is true, the entity will become simulated and visible. If its argument false, the entity will become unsimulated and invisible. The initial simulation and visibility state of the entity remains unchanged until the event is broadcast for the first time.";
					data = "AttributeSystemSubcategory";
					control = "KH_SubcategoryNoHeader4";
				};
				class KH_ConditionalPresence 
				{
					property = "KH_ConditionalPresence";
					control = "KH_ConditionalPresence";
					expression = 
					"\
						_value params ['_toggle', '_event', '_init', '_invert'];\
						if (_toggle && !is3DEN && (_event isNotEqualTo '')) then {\
							[_this, _event, compile _init, _invert] call KH_fnc_conditionalPresence;\
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
					description = "Converts this unit to an agent upon mission start. Most Eden attributes will be lost, but can be replicated using the Init field.";
					data = "AttributeSystemSubcategory";
					control = "KH_SubcategoryNoHeader2";
				};
				class KH_ConvertToAgent 
				{
					property = "KH_ConvertToAgent";
					control = "KH_ConvertToAgent";
					expression = 
					"\
						_value params ['_toggle', '_init'];\
						if (_toggle && !is3DEN) then {\
							KH_var_postInitExecutions pushBack [\
								[_this, compile _init],\
								{\
									call KH_fnc_convertToAgent;\
								}\
							];\
						};\
					";
					defaultValue = "[false, '']";
					condition = "objectControllable";
				};
			};
		};
		class KH_PersistencySettings
		{
			displayName = "KH Persistency Settings";
			collapsed = 1;
			class Attributes
			{
				class KH_PersistencySettingsSubcategory
				{
					description = "Settings that this entity will use for the persistency system from the mission attributes.";
					data = "AttributeSystemSubcategory";
					control = "KH_SubcategoryNoHeader1";
				};
				class KH_PersistencySettings
				{
					property = "KH_PersistencySettings";
					control = "KH_ObjectPersistencySettings";
					expression = 
					"\
						_value params ['_toggle', '_playerUseVariableName', '_ignoreTransforms'];\
						if (_toggle && !is3DEN) then {\
							if _playerUseVariableName then {
								_this setVariable ['KH_var_playerPersistencyUseVariableName', true];\
							};\
							if _ignoreTransforms then {\
								_this setVariable ['KH_var_persistencyIgnoreTransforms', true];\
							};\
						};\
					";
					defaultValue = "[false, false, false]";
				};
			};
		};
		class KH_PersistentInit
		{
			displayName = "KH Persistent Init";
			collapsed = 1;
			class Attributes
			{
				class KH_PersistentInitSubcategory
				{
					description = "Unscheduled code executed every time this entity changes locality, ideal for code that is dependent on locality.";
					data = "AttributeSystemSubcategory";
					control = "KH_SubcategoryNoHeader2";
				};
				class KH_PersistentInit 
				{
					property = "KH_PersistentInit";
					control = "KH_PersistentInit";
					expression = 
					"\
						_value params ['_toggle', '_localFunction', '_remoteFunction'];\
						if (_toggle && !is3DEN) then {\
							KH_var_postInitExecutions pushBack [\
								[_this, compile _localFunction, compile _remoteFunction],\
								{\
									params ['_entity', '_localFunction', '_remoteFunction'];\
									[\
										[_entity, _localFunction],\
										{\
											params ['_entity', '_localFunction'];\
											[_entity] call _localFunction;\
										},\
										_entity,\
										true,\
										[\
											'PERSISTENT',\
											true,\
											[_entity, _remoteFunction],\
											{\
												params ['_entity', '_remoteFunction'];\
												[_entity] call _remoteFunction;\
											},\
											''\
										]\
									] call KH_fnc_execute;\
								}\
							];\
						};\
					";
					defaultValue = "[false, '', '']";
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
						_value params ['_toggle', '_owner', '_recreate', '_init'];\
						if (_toggle && !is3DEN) then {\
							KH_var_headlessClientTransfers pushBack [_this, _owner, _recreate];\
							if (_init isNotEqualTo '') then {\
								_this setVariable ['KH_var_headlessClientTransferInit', compile _init];\
							};\
						};\
					";
					defaultValue = "[false, '', false, '']";
				};
			};
		};
	};
};