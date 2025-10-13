class KH_ModuleTeleport: Module_F
{
	scope = 2;
	scopeCurator = 1;
	canSetArea = 0;
	canSetAreaHeight = 0;
	canSetAreaShape = 0;
	displayName = "Teleporter";
	category = "KH_Modules";
	function = "KH_fnc_moduleTeleporter";
	functionPriority = 1;
	isGlobal = 0;
	isTriggerActivated = 1;
	isDisposable = 1;
	is3DEN = 0;
	icon = "iconcontrolpoint";
	class Attributes: AttributesBase
	{
		class KH_ModuleTeleportName: Edit
		{
			displayName = "Name";
			tooltip = "Name of the teleport action.";
			property = "KH_ModuleTeleportName";
			defaultValue = "''";
		};
		class KH_ModuleTeleportTransition: Edit
		{
			displayName = "Transition";
			tooltip = "Duration of a teleport transition, where the screen fades out and fades back in to hide the teleport, in seconds.";
			property = "KH_ModuleTeleportTransition";
			defaultValue = "''";
		};
		class KH_ModuleTeleportHeal: Checkbox
		{
			displayName = "Heal";
			tooltip = "<true> heals the teleporting entities after the teleport.";
			property = "KH_ModuleTeleportHeal";
			defaultValue = "false";
		};
		class KH_ModuleTeleportFreefallHeight: Edit
		{
			displayName = "Freefall Height";
			tooltip = "Freefall height assigned to a teleporting unit, in meters. Useful for cases where the destination is higher than the default freefall height of 100 meters. Set to -1 to reset, or set to 0 to avoid any freefall height change.";
			property = "KH_ModuleTeleportFreefallHeight";
			defaultValue = "''";
		};
		class KH_ModuleTeleportInit: Edit
		{
			displayName = "Init";
			tooltip = "Unscheduled code to execute locally to the entity after it is teleported. Passed arguments available through _this are: [_entity (OBJECT)].";
			property = "KH_ModuleTeleportInit";
			defaultValue = "''";
		};
		class ModuleDescription: ModuleDescription {};
	};
	class ModuleDescription: ModuleDescription
	{
		description[] = {"Adds a teleport action to the synchronized entities, allowing players to teleport to this module's position through the entities."};
		sync[] = {"Anything"};
		position = 1;
		direction = 1;
		optional = 0;
		duplicate = 1;
	};
};