class KH_ModuleTeleport: Module_F
{
	scope = 2;
	scopeCurator = 1;
	canSetArea = 0;
	canSetAreaHeight = 0;
	canSetAreaShape = 0;
	displayName = "Teleport";
	category = "KH_Modules";
	function = "KH_fnc_moduleTeleport";
	functionPriority = 1;
	isGlobal = 0;
	isTriggerActivated = 1;
	isDisposable = 0;
	is3DEN = 0;
	icon = "iconcontrolpoint";
	curatorInfoType = "";
	curatorInfoTypeEmpty = "";
	curatorCanAttach = 0;
	class Attributes: AttributesBase
	{
		class KH_ModuleTeleportEject: Checkbox
		{
			displayName = "Eject";
			tooltip = "<true> allows the teleportation of units that are in vehicles.";
			property = "KH_ModuleTeleportEject";
			defaultValue = "false";
		};
		class KH_ModuleTeleportTransition: Edit
		{
			displayName = "Transition";
			tooltip = "Duration of a teleport transition, where the screen fades out and fades back in to hide the teleport, in seconds. Only has an effect if the teleporting entity is a player.";
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
			tooltip = "Freefall height assigned to a teleporting unit, in meters. useful for cases where the destination is higher than the default freefall height of 100 meters. Set to <-1> to reset, or set to <0> to avoid any freefall height change.";
			property = "KH_ModuleTeleportFreefallHeight";
			defaultValue = "''";
		};
		class KH_ModuleTeleportInitialization: Edit
		{
			displayName = "Initialization";
			tooltip = "Unscheduled code to execute locally to the entity after it is teleported. Passed arguments available through <_this> are: <[_entity (OBJECT)]>.";
			property = "KH_ModuleTeleportInitialization";
			defaultValue = "''";
		};
		class ModuleDescription: ModuleDescription {};
	};
	class ModuleDescription: ModuleDescription
	{
		description[] = {"Teleports the synchronized entities to this module's position. Can be exclusively activated through a trigger or triggers. Can activate multiple times. Activated on the server."};
		sync[] = {"Anything"};
		position = 1;
		direction = 1;
		optional = 0;
		duplicate = 1;
	};
};