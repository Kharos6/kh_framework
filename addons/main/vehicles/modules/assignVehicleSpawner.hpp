class KH_ModuleAssignVehicleSpawner: Module_F
{
	author = "Kharos";
	scope = 2;
	scopeCurator = 1;
	canSetArea = 0;
	canSetAreaHeight = 0;
	canSetAreaShape = 0;
	displayName = "Assign Vehicle Spawner";
	category = "KH_Modules";
	function = "KH_fnc_moduleAssignVehicleSpawner";
	functionPriority = 1;
	isGlobal = 0;
	isTriggerActivated = 0;
	isDisposable = 0;
	is3DEN = 0;
	curatorInfoType = "";
	curatorInfoTypeEmpty = "";
	curatorCanAttach = 0;
	icon = "\a3\Missions_F_Curator\data\img\iconMPTypeSectorControl_ca.paa";
	class Attributes: AttributesBase
	{
		class KH_ModuleAssignVehicleSpawnerVehicleTypes: EditMulti5
		{
			displayName = "Vehicle Types";
			tooltip = "Strings of class names of entity types from CfgVehicles that will be available in the spawn menu.";
			property = "KH_ModuleAssignVehicleSpawnerVehicleTypes";
			defaultValue = "''";
		};
		class ModuleDescription: ModuleDescription {};
	};
	class ModuleDescription: ModuleDescription
	{
		description[] = {"Creates an accessible menu on the synchronized entities that allows players to spawn vehicles on the module's position. Activates only once, without a trigger, on the server."};
		sync[] = {};
		position = 0;
		direction = 1;
		optional = 0;
		duplicate = 1;
	};
};