class KH_ModuleAddInteractionEntry: Module_F
{
	scope = 2;
	scopeCurator = 1;
	canSetArea = 0;
	canSetAreaHeight = 0;
	canSetAreaShape = 0;
	displayName = "Add Interaction Entry";
	category = "KH_Modules";
	function = "KH_fnc_moduleAddInteractionEntry";
	functionPriority = 1;
	isGlobal = 2;
	isTriggerActivated = 0;
	isDisposable = 1;
	is3DEN = 0;
	icon = "iconmodule";
	curatorInfoType = "";
	curatorInfoTypeEmpty = "";
	curatorCanAttach = 0;
	class Attributes: AttributesBase
	{
		class KH_ModuleAddInteractionEntryRemote: Checkbox
		{
			displayName = "Remote";
			tooltip = "<true> makes the interaction entry remote, meaning that it will be displayed when another player opens the remote interaction menu, accessible through the right <windows> key, when looking at this entity. <false> means that the interaction will be displayed only to this player, when this player opens their own interaction menu, accessible through the combination of the right <windows> key and <ALT>. Only works if this entity is a playable unit.";
			property = "KH_ModuleAddInteractionEntryRemote";
			defaultValue = "false";
		};
		class KH_ModuleAddInteractionEntryName: Edit
		{
			displayName = "Name";
			tooltip = "Name of the interaction entry.";
			property = "KH_ModuleAddInteractionEntryName";
			defaultValue = "''";
		};
		class KH_ModuleAddInteractionEntryCondition: Edit
		{
			displayName = "Condition";
			tooltip = "Unscheduled code, executed locally to the player calling the interaction menu, which must return <true> in order for this entry to appear. If <Remote> is <true>, passed arguments available through <_this> in <_function> are: <[_entity (OBJECT)]>.";
			property = "KH_ModuleAddInteractionEntryCondition";
			defaultValue = "''";
		};
		class KH_ModuleAddInteractionEntryOptions: Edit
		{
			displayName = "Options";
			tooltip = "Array of arrays dictating the possible options available for this interaction entry through a context menu, in format: <[_name (STRING), _condition (STRING), _function (STRING)]>, or a <missionNamespace> variable containing the same. If <Remote> is <true>, variable <'KH_var_currentInteractionTarget'> can be used in <_condition> and <_function> to retrieve the entity being interacted with.";
			property = "KH_ModuleAddInteractionEntryOptions";
			defaultValue = "[]";
		};
		class ModuleDescription: ModuleDescription {};
	};
	class ModuleDescription: ModuleDescription
	{
		description[] = {"Adds an interaction menu entry to the synchronized entities. Activates without a trigger. Can activate only once. Activated locally to each player."};
		sync[] = {"Anything"};
		position = 0;
		direction = 0;
		optional = 0;
		duplicate = 1;
	};
};