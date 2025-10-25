class KH_ModuleTeleporter: Module_F
{
	author = "Kharos";
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
	isTriggerActivated = 0;
	isDisposable = 1;
	is3DEN = 0;
	curatorInfoType = "";
	curatorInfoTypeEmpty = "";
	curatorCanAttach = 0;
	icon = "iconcontrolpoint";
	class Attributes: AttributesBase
	{
		class KH_ModuleTeleporterName: Edit
		{
			displayName = "Name";
			tooltip = "Name of the teleport action.";
			property = "KH_ModuleTeleporterName";
			defaultValue = "''";
		};
		class KH_ModuleTeleporterTransition: Edit
		{
			displayName = "Transition";
			tooltip = "Duration of a teleport transition, where the screen fades out and fades back in to hide the teleport, in seconds.";
			property = "KH_ModuleTeleporterTransition";
			defaultValue = "'1'";
		};
		class KH_ModuleTeleporterFreefallHeight: Edit
		{
			displayName = "Freefall Height";
			tooltip = "Freefall height assigned to a teleporting unit, in meters. Useful for cases where the destination is higher than the default freefall height of 100 meters. Set to -1 to reset, or set to 0 to avoid any freefall height change.";
			property = "KH_ModuleTeleporterFreefallHeight";
			defaultValue = "'-1'";
		};
		class KH_ModuleTeleporterInit: EditCodeMulti5
		{
			displayName = "Init";
			tooltip = "Unscheduled code to execute locally to the entity after it is teleported. Passed arguments available through _this are: [_teleportingPerson (OBJECT)].";
			property = "KH_ModuleTeleporterInit";
			defaultValue = "''";
		};
		class ModuleDescription: ModuleDescription {};
	};
	class ModuleDescription: ModuleDescription
	{
		description[] = {"Adds a teleport action to the synchronized entities, allowing players to teleport to this module's position through the entities. Activates only once, without a trigger, on the server."};
		sync[] = {"Anything"};
		position = 1;
		direction = 1;
		optional = 0;
		duplicate = 1;
	};
};