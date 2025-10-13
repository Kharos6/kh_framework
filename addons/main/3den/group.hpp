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
					tooltip = "Specify an array of strings of global variables that will be made into an array, made public, and contain this group and any other groups utilizing this function. In format <['globalVariable1', 'globalVariable2', 'globalVariable3']>";
					property = "KH_ArrayBuilder";
					control = "Edit";
					expression = 
					"\
						if ((_value isNotEqualTo '') && (_value isNotEqualTo '[]') && !is3DEN) then {\
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
					defaultValue = "'[]'";
				};
				class KH_IgnoredTargets
				{
					displayName = "Ignored Targets";
					tooltip = "Specify an array of strings of variable names of entities to be ignored as potential targets by the members of this group.";
					property = "KH_IgnoredTargets";
					control = "Edit";
					expression = 
					"\
						if ((_value isNotEqualTo '') && (_value isNotEqualTo '[]') && !is3DEN) then {\
							KH_var_postInitExecutions pushBack [\
								[_this, parseSimpleArray _value],\
								{\
									params ['_group', '_targets'];\
									{\
										_group ignoreTarget [missionNamespace getVariable [_x, objNull], true];\
									} forEach _targets;\
								}\
							];\
						};\
					";
					defaultValue = "'[]'";
				};
				class KH_PlayerGroupInit
				{
					displayName = "Player Group Init";
					tooltip = "Unscheduled code to execute locally to the players who loads into the mission as members of this group, once their unit becomes valid. Passed arguments available through _this are: [_group (GROUP)].";
					property = "KH_PlayerGroupInit";
					control = "EditMulti5";
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
				};
				class KH_ServerGroupInit
				{
					displayName = "Server Group Init";
					tooltip = "Unscheduled code to execute on the server with this group passed as an argument. Passed arguments available through _this are: [_group (GROUP)].";
					property = "KH_ServerGroupInit";
					control = "EditMulti5";
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
				};
			};
		};
	};
};