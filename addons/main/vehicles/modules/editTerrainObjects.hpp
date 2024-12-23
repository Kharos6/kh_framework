class KH_ModuleEditTerrainObjects: Module_F
{
	scope = 2;
	scopeCurator = 1;
	canSetArea = 1;
	canSetAreaHeight = 1;
	canSetAreaShape = 1;
	displayName = "Edit Terrain Objects";
	category = "KH_Modules";
	function = "KH_fnc_moduleEditTerrainObjects";
	functionPriority = 1;
	isGlobal = 0;
	isTriggerActivated = 1;
	isDisposable = 1;
	is3DEN = 0;
	icon = "\a3\modules_f\data\editterrainobject\icon32_ca.paa";
	curatorInfoType = "";
	curatorInfoTypeEmpty = "";
	curatorCanAttach = 0;
	class AttributeValues
	{
		size3[] = { 100, 100, -1 };
		isRectangle = 0;
	};
	class Attributes: AttributesBase
	{
		class KH_ModuleEditTerrainObjectsTypes: Edit
		{
			displayName = "Objects";
			tooltip = "Array of strings of class names of objects that will be edited. Can be an empty array to edit all object types.";
			property = "KH_ModuleEditTerrainObjectsTypes";
			defaultValue = "'[]'";
		};
		class KH_ModuleEditTerrainObjectsDamage: Edit
		{
			displayName = "Damage";
			tooltip = "Damage that is applied to the affected objects, calculated per object, from <0> to <1>.";
			property = "KH_ModuleEditTerrainObjectsDamage";
			defaultValue = "''";
		};
		class KH_ModuleEditTerrainObjectsChance: Edit
		{
			displayName = "Chance";
			tooltip = "Chance for an object to be affected, calculated per object, from <0> to <1>.";
			property = "KH_ModuleEditTerrainObjectsChance";
			defaultValue = "''";
		};
		class KH_ModuleEditTerrainObjectsRandomDamage: Checkbox
		{
			displayName = "Random Damage";
			tooltip = "<true> applies a random damage amount to each affected object, with the maximum possible damage value derived from <Damage>.";
			property = "KH_ModuleEditTerrainObjectsRandomDamage";
			defaultValue = "false";
		};
		class KH_ModuleEditTerrainObjectsEffects: Checkbox
		{
			displayName = "Effects";
			tooltip = "<true> allows destruction effects to trigger on the affected objects when they are damaged by this function.";
			property = "KH_ModuleEditTerrainObjectsEffects";
			defaultValue = "false";
		};
		class KH_ModuleEditTerrainObjectsDynamicSimulation: Checkbox
		{
			displayName = "Dynamic Simulation";
			tooltip = "<true> enables dynamic simulation on the affected objects.";
			property = "KH_ModuleEditTerrainObjectsDynamicSimulation";
			defaultValue = "false";
		};
		class KH_ModuleEditTerrainObjectsDisableDamage: Checkbox
		{
			displayName = "Disable Damage";
			tooltip = "<true> prevents the affected objects from receiving damage. Does not prevent damage from being applied by this function.";
			property = "KH_ModuleEditTerrainObjectsDisableDamage";
			defaultValue = "false";
		};
		class KH_ModuleEditTerrainObjectsConvertToSimple: Checkbox
		{
			displayName = "Convert To Simple";
			tooltip = "<true> converts the affected objects to simple objects.";
			property = "KH_ModuleEditTerrainObjectsConvertToSimple";
			defaultValue = "false";
		};
		class KH_ModuleEditTerrainObjectsInitialization: Edit
		{
			displayName = "Initialization";
			tooltip = "Unscheduled code to execute on edited entities, executed on the server. Passed arguments available through <_this> are: <[_entity (OBJECT)]>";
			property = "KH_ModuleEditTerrainObjectsInitialization";
			defaultValue = "''";
		};
		class ModuleDescription: ModuleDescription {};
	};
	class ModuleDescription: ModuleDescription
	{
		description[] = {"Edits terrain objects within this module's area. Can be exclusively activated through a trigger or triggers. Can activate only once. Activated on the server."};
		position = 1;
		direction = 1;
	};
};