class KH_ModuleAddAction: Module_F
{
	author = "Kharos";
	scope = 2;
	scopeCurator = 1;
	canSetArea = 0;
	canSetAreaHeight = 0;
	canSetAreaShape = 0;
	displayName = "Add Action";
	category = "KH_Modules";
	function = "KH_fnc_moduleAddAction";
	functionPriority = 1;
	isGlobal = 0;
	isTriggerActivated = 0;
	isDisposable = 1;
	is3DEN = 0;
	curatorInfoType = "";
	curatorInfoTypeEmpty = "";
	curatorCanAttach = 0;
	icon = "iconmodule";
	class Attributes: AttributesBase
	{
		class KH_ModuleAddActionName: Edit
		{
			displayName = "Name";
			tooltip = "Name of the action.";
			property = "KH_ModuleAddActionName";
			defaultValue = "''";
		};
		class KH_ModuleAddActionFunctionStart: EditCodeMulti5
		{
			displayName = "Function Start";
			tooltip = "Unscheduled code executed locally to the caller when the caller initiates the action.";
			property = "KH_ModuleAddActionFunctionStart";
			defaultValue = "''";
		};
		class KH_ModuleAddActionFunctionProgress: EditCodeMulti5
		{
			displayName = "Function Progress";
			tooltip = "Unscheduled code executed locally to the caller on each frame as the action progresses through its duration.";
			property = "KH_ModuleAddActionFunctionProgress";
			defaultValue = "''";
		};
		class KH_ModuleAddActionFunctionInterrupt: EditCodeMulti5
		{
			displayName = "Function Interrupt";
			tooltip = "Unscheduled code executed locally to the caller when the action is interrupted in the event of a condition failure or the caller moving beyond the designated distance.";
			property = "KH_ModuleAddActionFunctionInterrupt";
			defaultValue = "''";
		};
		class KH_ModuleAddActionFunctionComplete: EditCodeMulti5
		{
			displayName = "Function Complete";
			tooltip = "Unscheduled code executed locally to the caller when the action is completed.";
			property = "KH_ModuleAddActionFunctionComplete";
			defaultValue = "''";
		};
		class KH_ModuleAddActionConditionShow: EditCodeMulti5
		{
			displayName = "Condition Show";
			tooltip = "Unscheduled code, executed locally to the caller, that must return true in order for the action to be visible.";
			property = "KH_ModuleAddActionConditionShow";
			defaultValue = "'true'";
		};
		class KH_ModuleAddActionConditionProgress: EditCodeMulti5
		{
			displayName = "Condition Progress";
			tooltip = "Unscheduled code, executed locally to the caller, that must return true in order for the action to progress through its duration.";
			property = "KH_ModuleAddActionConditionProgress";
			defaultValue = "'true'";
		};
		class KH_ModuleAddActionConditionComplete: EditCodeMulti5
		{
			displayName = "Condition Complete";
			tooltip = "Unscheduled code, executed locally to the caller, that must return true in order for the action to be allowed to complete.";
			property = "KH_ModuleAddActionConditionComplete";
			defaultValue = "'true'";
		};
		class KH_ModuleAddActionRepeatable: Checkbox
		{
			displayName = "Repeatable";
			tooltip = "True allows the action to be called recursively after completion.";
			property = "KH_ModuleAddActionRepeatable";
			defaultValue = "false";
		};
		class KH_ModuleAddActionExclusive: Checkbox
		{
			displayName = "Repeatable";
			tooltip = "True prevents multiple players from being able to call the action at the same time.";
			property = "KH_ModuleAddActionExclusive";
			defaultValue = "true";
		};
		class KH_ModuleAddActionDuration: Edit
		{
			displayName = "Duration";
			tooltip = "Duration of the action call before completion. Set to 0 for instant completion, still subject to the conditions.";
			property = "KH_ModuleAddActionDuration";
			defaultValue = "'0'";
		};
		class KH_ModuleAddActionDistance: Edit
		{
			displayName = "Distance";
			tooltip = "Distance within which the caller must be to the closest surface of the synchronized entities in order for the action to be usable.";
			property = "KH_ModuleAddActionDistance";
			defaultValue = "'3'";
		};
		class KH_ModuleAddActionShowImmediately: Checkbox
		{
			displayName = "Show Immediately";
			tooltip = "True immediately displays the action upon it becoming usable. False requires the user to actively scroll through the action menu to see it.";
			property = "KH_ModuleAddActionShowImmediately";
			defaultValue = "true";
		};
		class KH_ModuleAddActionShowUnconscious: Checkbox
		{
			displayName = "Show Unconscious";
			tooltip = "True allows the action to be usable while the caller is unconscious.";
			property = "KH_ModuleAddActionShowUnconscious";
			defaultValue = "false";
		};
		class KH_ModuleAddActionHideOnUse: Checkbox
		{
			displayName = "Hide On Use";
			tooltip = "True hides the action when it is used. This does not remove the action, but rather deselects it. This includes actions that have a duration.";
			property = "KH_ModuleAddActionHideOnUse";
			defaultValue = "false";
		};
		class KH_ModuleAddActionDrawHint: Checkbox
		{
			displayName = "Draw Hint";
			tooltip = "Displays a small interaction hint in 3D space that makes the presence of the action more clear to potential callers.";
			property = "KH_ModuleAddActionDrawHint";
			defaultValue = "false";
		};
		class KH_ModuleAddActionDetection: Checkbox
		{
			displayName = "Detection";
			tooltip = "True makes it so that the caller must be looking at the surface of a synchronized entity in order for the action to be usable. False makes it so that the action is usable at any point for as long as the user is within the designated distance of a synchronized entity.";
			property = "KH_ModuleAddActionDetection";
			defaultValue = "true";
		};
		class KH_ModuleAddActionProgressDisplay: Combo
		{
			displayName = "Progress Display";
			tooltip = "The type of visual progress indicator to display to the caller through the duration of the action.";
			property = "KH_ModuleAddActionProgressDisplay";
			typeName = "STRING";
			defaultValue = "'BAR'";
			class Values
			{
				class KH_None {name = "NONE"; value = "NONE";};
				class KH_Bar {name = "BAR"; value = "BAR";};
				class KH_Percentage {name = "PERCENTAGE"; value = "PERCENTAGE";};
				class KH_Time {name = "TIME"; value = "TIME";};
			};
		};
		class KH_ModuleAddActionIdentifier: Edit
		{
			displayName = "Identifier";
			tooltip = "A string that is otherwise unique per action, but can be overriden by this attribute; if used, all actions tied to this identifier will share the same exclusivity and existence conditions. For example, if the action has a duration, exclusive is true, and repeatable is false, and two actions share the same identifier, then as soon as one of those actions is activated, the other one will not be usable and will be deleted when the first one completes. Leave empty to generate a unique identifier per action.";
			property = "KH_ModuleAddActionIdentifier";
			defaultValue = "''";
		};
		class ModuleDescription: ModuleDescription {};
	};
	class ModuleDescription: ModuleDescription
	{
		description[] = {"Adds an action to the synchronized entities. Activates only once, without a trigger, on the server."};
		sync[] = {"Anything"};
		position = 0;
		direction = 0;
		optional = 0;
		duplicate = 1;
	};
};