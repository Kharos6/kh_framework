class Group
{
	class AttributeCategories
	{
		class KH_Miscellaneous
		{
			displayName = "KH Miscellaneous";
			collapsed = 1;
			class Attributes
			{
				class KH_ArrayBuilder
				{
					displayName = "Array Builder";
					tooltip = "Strings of global variables that will be made into an array, made public, and contain this group and any other groups utilizing this function. In format 'globalVariable1', 'globalVariable2', 'globalVariable3', ....";
					property = "KH_ArrayBuilder";
					control = "Edit";
					expression = 
					"\
						if ((_value isNotEqualTo '') && !is3DEN) then {\
							_value = ['[', _value, ']'] joinString '';\
							{\
								private _originalValueArray = missionNamespace getVariable [_x, []];\
								_originalValueArray pushBack _this;\
								missionNamespace setVariable [_x, _originalValueArray];\
								private _originalArrayArray = missionNamespace getVariable ['KH_var_groupArrayBuilderArrays', []];\
								_originalArrayArray pushBackUnique _x;\
								missionNamespace setVariable ['KH_var_groupArrayBuilderArrays', _originalArrayArray];\
							} forEach (parseSimpleArray _value);\
						};\
					";
					defaultValue = "''";
				};
				class KH_IgnoredTargets
				{
					displayName = "Ignored Targets";
					tooltip = "Strings of variable names of units or groups to be ignored as potential targets by the members of this group.";
					property = "KH_IgnoredTargets";
					control = "Edit";
					expression = 
					"\
						if ((_value isNotEqualTo '') && !is3DEN) then {\
							_value = ['[', _value, ']'] joinString '';\
							KH_var_postInitExecutions pushBack [\
								[_this, parseSimpleArray _value],\
								{\
									params ['_group', '_targets'];\
									{\
										_group ignoreTarget (missionNamespace getVariable [_x, objNull]);\
									} forEach _targets;\
								}\
							];\
						};\
					";
					defaultValue = "''";
				};
				class KH_PlayerGroupInit
				{
					displayName = "Player Group Init";
					tooltip = "Unscheduled code to execute locally to the players who load into the mission as members of this group once the player unit becomes available. Passed arguments available through _this are: [_group (GROUP)].";
					property = "KH_PlayerGroupInit";
					control = "EditCodeMulti5";
					expression = 
					"\
						if ((_value isNotEqualTo '') && !is3DEN) then {\
							KH_var_postInitExecutions pushBack [\
								[_this, compile _value],\
								{\
									params ['_group', '_function'];\
									[\
										'CBA',\
										'KH_eve_playerLoaded',\
										[_group, _function],\
										{\
											_args params ['_group', '_function'];\
											if ((group player) isEqualTo _group) then {\
												[_group] call _function;\
											};\
										}\
									] call KH_fnc_addEventHandler;\
								}\
							];\
						};\
					";
					defaultValue = "''";
					validate = "expression";
				};
				class KH_ServerGroupInit
				{
					displayName = "Server Group Init";
					tooltip = "Unscheduled code to execute on the server with this group passed as an argument. Passed arguments available through _this are: [_group (GROUP)].";
					property = "KH_ServerGroupInit";
					control = "EditCodeMulti5";
					expression = 
					"\
						if ((_value isNotEqualTo '') && !is3DEN) then {\
							KH_var_postInitExecutions pushBack [\
								[_this, compile _value],\
								{\
									params ['_group', '_function'];\
									[_group] call _function;\
								}\
							];\
						};\
					";
					defaultValue = "''";
					validate = "expression";
				};
			};
		};
	};
};