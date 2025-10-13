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
					description = "Specify a CBA event which, every time it is broadcasted to the server, changes the simulation and visibility of this entity. If its argument is true, the entity will become simulated and visible. If its argument false, the entity will become unsimulated and invisible. The initial simulation and visibility state of the entity remains unchanged until the event is broadcast for the first time.";
					data = "AttributeSystemSubcategory";
					control = "KH_SubcategoryNoHeader4";
				};
				class KH_ConditionalPresence 
				{
					property = "KH_ConditionalPresence";
					control = "KH_ConditionalPresence";
					expression = 
					"\
						_value params ['_toggle', '_variableName', '_init', '_invert'];\
						if (_toggle && !is3DEN && (_variableName isNotEqualTo '')) then {\
							[_this, _variableName, compile _init, _invert] call KH_fnc_conditionalPresence;\
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
								[[_this], compile _init],\
								{\
									_this call KH_fnc_convertToAgent;\
								}\
							];\
						};\
					";
					defaultValue = "[false, '']";
					condition = "objectControllable";
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
					description = "Execute code every time this entity changes locality, ideal for code that is dependent on locality.";
					data = "AttributeSystemSubcategory";
					control = "KH_SubcategoryNoHeader1";
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
										[\
											'PERSISTENT',\
											_entity,\
											[_entity, _remoteFunction],\
											{\
												params ['_entity', '_remoteFunction'];\
												[_entity] call _remoteFunction;\
											},\
											true\
										],\
										'THIS_FRAME'\
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
						if ((_value isNotEqualTo '') && !is3DEN) then {\
							KH_var_postInitExecutions pushBack [\
								[_this, parseNumber _value],\
								{\
									[\
										_this,\
										{\
											params ['_unit', '_speed'];\
											_unit setAnimSpeedCoef _speed;\
										},\
										['JIP', 'PLAYERS', _this select 0, false, false, ''],\
										'THIS_FRAME'\
									] call KH_fnc_execute;\
								}\
							];\
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
						if ((_value isNotEqualTo '') && (_value isNotEqualTo '[]') && !is3DEN) then {\
							private _array = parseSimpleArray _value;\
							_this setVariable ['KH_var_assignedEntityArrayBuilderArrays', _array];\
							private _originalArrayBuilderArray = missionNamespace getVariable ['KH_var_entityArrayBuilderArrays', []];\
							{\
								private _originalValueArray = missionNamespace getVariable [_x, []];\
								_originalValueArray pushBack _this;\
								missionNamespace setVariable [_x, _originalValueArray];\
								_originalArrayBuilderArray pushBackUnique _x;\
								missionNamespace setVariable ['KH_var_entityArrayBuilderArrays', _originalArrayBuilderArray];\
							} forEach _array;\
						};\
					";
					defaultValue = "'[]'";
				};
				class KH_FreefallHeight
				{
					displayName = "Freefall Height";
					tooltip = "Height above which this unit will enter freefall, in meters. Leave empty for no change from the default of 100 metres.";
					property = "KH_FreefallHeight";
					control = "Edit";
					expression = 
					"\
						if ((_value isNotEqualTo '') && !is3DEN) then {\
							_this setUnitFreefallHeight (parseNumber _value);\
						};\
					";
					defaultValue = "''";
					condition = "objectControllable";
				};
				class KH_PlayerObjectInit
				{
					displayName = "Player Object Init";
					tooltip = "Unscheduled code to execute locally to the player who loads into the mission as this unit, once the unit becomes valid. Only works if this unit is playable. The local <player> variable is valid as this unit.";
					property = "KH_PlayerObjectInit";
					control = "EditMulti5";
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
											if (player == _entity) then {\
												[] call _function;\
											};\
										}\
									] call KH_fnc_addEventHandler;\
								}\
							];\
						};\
					";
					defaultValue = "''";
					condition = "objectControllable";
				};
				class KH_ServerObjectInit
				{
					displayName = "Server Object Init";
					tooltip = "Unscheduled code to execute on the server with this entity passed as an argument. Passed arguments available through <_this> are: <[_entity (OBJECT)]>.";
					property = "KH_ServerObjectInit";
					control = "EditMulti5";
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
							KH_var_postInitExecutions pushBack [\
								[_this],\
								{\
									[\
										_this,\
										{\
											params ['_entity'];\
											_entity lockInventory true;\
										},\
										['JIP', 'PLAYERS', _this select 0, false, false, ''],\
										'THIS_FRAME'\
									] call KH_fnc_execute;\
								}\
							];\
						};\
					";
					defaultValue = "false";
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
										[[_entity], parseSimpleArray _loadouts] call KH_fnc_setRandomLoadout;\
									}\
									else {\
										[[_entity], missionNamespace getVariable [_loadouts, []]] call KH_fnc_setRandomLoadout;\
									};\
								}\
							];\
						};\
					";
					defaultValue = "''";
					condition = "objectControllable";
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
							_this addRating -100000;\
						};\
					";
					defaultValue = "false";
					condition = "objectControllable";
				};
			};
		};
	};
};