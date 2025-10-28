class KH_ModuleVisualTrigger: Module_F
{
	author = "Kharos";
	scope = 2;
	scopeCurator = 1;
	canSetArea = 0;
	canSetAreaHeight = 0;
	canSetAreaShape = 0;
	displayName = "Visual Trigger";
	category = "KH_Modules";
	function = "KH_fnc_moduleVisualTrigger";
	functionPriority = 1;
	isGlobal = 0;
	isTriggerActivated = 0;
	isDisposable = 0;
	is3DEN = 0;
	curatorInfoType = "";
	curatorInfoTypeEmpty = "";
	curatorCanAttach = 0;
	icon = "iconLogic";
	class Attributes: AttributesBase
	{
		class KH_ModuleVisualTriggerScreenPercentage: Edit
		{
			displayName = "Screen Percentage";
			tooltip = "The distance from the edge of the screen to the center of the screen within which the trigger will activate, from 0 to 1, where 0 is pin point center and 1 is the whole screen.";
			property = "KH_ModuleVisualTriggerScreenPercentage";
			defaultValue = "'0.5'";
		};
		class KH_ModuleVisualTriggerMinimumDistance: Edit
		{
			displayName = "Minimum Distance";
			tooltip = "The distance within which the trigger will activate, regardless of line of sight.";
			property = "KH_ModuleVisualTriggerMinimumDistance";
			defaultValue = "'1'";
		};
		class KH_ModuleVisualTriggerMaximumDistance: Edit
		{
			displayName = "Maximum Distance";
			tooltip = "The distance beyond which the trigger will not activate, regardless of line of sight.";
			property = "KH_ModuleVisualTriggerMaximumDistance";
			defaultValue = "'1000'";
		};
		class KH_ModuleVisualTriggerConditionServer: EditCodeMulti5
		{
			displayName = "Condition Server";
			tooltip = "Unscheduled code executed locally to the server that must return true in order for the trigger to activate. Both this and Condition Player must be true. Passed arguments available through _this are: [_entity (OBJECT)]";
			property = "KH_ModuleVisualTriggerConditionServer";
			defaultValue = "'true'";
		};
		class KH_ModuleVisualTriggerConditionPlayer: EditCodeMulti5
		{
			displayName = "Condition Player";
			tooltip = "Unscheduled code executed locally to the observing player that must return true in order for the trigger to activate. Both this and Condition Server must be true. Passed arguments available through _this are: [_entity (OBJECT)]";
			property = "KH_ModuleVisualTriggerConditionPlayer";
			defaultValue = "'true'";
		};
		class KH_ModuleVisualTriggerTrueFunctionServer: EditCodeMulti5
		{
			displayName = "True Function Server";
			tooltip = "Unscheduled code executed locally to the server when the trigger is activated. Passed arguments available through _this are: [_activatingPlayer (OBJECT), _entity (OBJECT)]";
			property = "KH_ModuleVisualTriggerTrueFunctionServer";
			defaultValue = "''";
		};
		class KH_ModuleVisualTriggerFalseFunctionServer: EditCodeMulti5
		{
			displayName = "False Function Server";
			tooltip = "Unscheduled code executed locally to the server when the trigger is deactivated. Passed arguments available through _this are: [_activatingPlayer (OBJECT), _entity (OBJECT)]";
			property = "KH_ModuleVisualTriggerFalseFunctionServer";
			defaultValue = "''";
		};
		class KH_ModuleVisualTriggerTrueFunctionPlayer: EditCodeMulti5
		{
			displayName = "True Function Player";
			tooltip = "Unscheduled code executed locally to the player when the trigger is activated. Passed arguments available through _this are: [_entity (OBJECT)]";
			property = "KH_ModuleVisualTriggerTrueFunctionPlayer";
			defaultValue = "''";
		};
		class KH_ModuleVisualTriggerFalseFunctionPlayer: EditCodeMulti5
		{
			displayName = "False Function Player";
			tooltip = "Unscheduled code executed locally to the player when the trigger is deactivated. Passed arguments available through _this are: [_entity (OBJECT)]";
			property = "KH_ModuleVisualTriggerFalseFunctionPlayer";
			defaultValue = "''";
		};
		class KH_ModuleVisualTriggerRepeatable: Checkbox
		{
			displayName = "Repeatable";
			tooltip = "True allows the trigger to be recursively activated and deactivated.";
			property = "KH_ModuleVisualTriggerRepeatable";
			defaultValue = "false";
		};
		class KH_ModuleVisualTriggerInterval: Edit
		{
			displayName = "Interval";
			tooltip = "Interval at which the trigger may activate.";
			property = "KH_ModuleVisualTriggerInterval";
			defaultValue = "'0.5'";
		};
		class KH_ModuleVisualTriggerShared: Checkbox
		{
			displayName = "Shared";
			tooltip = "True makes it so that once one player activates the trigger, it cannot be deactivated until all players are looking away from it. False makes it so that each player can activate and deactivate the trigger independently.";
			property = "KH_ModuleVisualTriggerShared";
			defaultValue = "true";
		};
		class ModuleDescription: ModuleDescription {};
	};
	class ModuleDescription: ModuleDescription
	{
		description[] = {"Makes the synchronized entities act as a visual trigger, executing unscheduled code on activation and deactivation when a player observes the object with clear line of sight, for as long as the module exists. Activates only once, without a trigger, on the server."};
		sync[] = {"Anything"};
		position = 0;
		direction = 0;
		optional = 0;
		duplicate = 1;
	};
};