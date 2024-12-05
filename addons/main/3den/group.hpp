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
					tooltip = "Specify one or more strings of global variables that will be made into an array, made public, and contain this group and any other groups utilizing this function. In format ['globalVariable1', 'globalVariable2', 'globalVariable3']";
					property = "KH_ArrayBuilder";
					control = "Edit";
					expression = 
					"\
						if ((_value != '') && (_value != '[]')) then {\
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
			};
		};
	};
};