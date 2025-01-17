class KH_ModuleManageSimulatedInventory: Module_F
{
	scope = 2;
	scopeCurator = 1;
	canSetArea = 0;
	canSetAreaHeight = 0;
	canSetAreaShape = 0;
	displayName = "Manage Simulated Inventory";
	category = "KH_Modules";
	function = "KH_fnc_moduleManageSimulatedInventory";
	functionPriority = 1;
	isGlobal = 0;
	isTriggerActivated = 0;
	isDisposable = 0;
	is3DEN = 0;
	icon = "iconmodule";
	curatorInfoType = "";
	curatorInfoTypeEmpty = "";
	curatorCanAttach = 0;
	class Attributes: AttributesBase
	{
		class KH_ModuleManageSimulatedInventoryName: Edit
		{
			displayName = "Name";
			tooltip = "Name of the item to modify.";
			property = "KH_ModuleManageSimulatedInventoryName";
			defaultValue = "''";
		};
		class KH_ModuleManageSimulatedInventoryDescription: Edit
		{
			displayName = "Description";
			tooltip = "Description of the item. Can be left empty to retain original description.";
			property = "KH_ModuleManageSimulatedInventoryDescription";
			defaultValue = "''";
		};
		class KH_ModuleManageSimulatedInventoryAmount: Edit
		{
			displayName = "Amount";
			tooltip = "Amount of this item to add or subtract.";
			property = "KH_ModuleManageSimulatedInventoryAmount";
			defaultValue = "''";
		};
		class KH_ModuleManageSimulatedInventoryForceAmount: Checkbox
		{
			displayName = "Force Amount";
			tooltip = "<true> forces the amount to the set <Amount> value. <false> increments or decrements the amount by <Amount>.";
			property = "KH_ModuleManageSimulatedInventoryForceAmount";
			defaultValue = "false";
		};
		class ModuleDescription: ModuleDescription {};
	};
	class ModuleDescription: ModuleDescription
	{
		description[] = {"Adds, removes, or modifies items in the simulated inventory of synchronized entities. The simulated inventory of other entities is accessible through the combination of the right <windows> key and the <CTRL> key, while the local player inventory is accessible through the combination of the right <windows> key, the <CTRL> key, and the <ALT> key. Activates without a trigger. Can activate multiple times. Activated on the server."};
		sync[] = {"Anything"};
		position = 0;
		direction = 0;
		optional = 0;
		duplicate = 1;
	};
};