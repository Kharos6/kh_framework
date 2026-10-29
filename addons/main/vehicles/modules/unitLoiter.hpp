class KH_ModuleUnitLoiter: Module_F
{
	author = "Kharos";
	scope = 2;
	scopeCurator = 1;
	canSetArea = 0;
	canSetAreaHeight = 0;
	canSetAreaShape = 0;
	displayName = "Unit Loiter";
	category = "KH_Modules";
	function = "KH_fnc_moduleUnitLoiter";
	functionPriority = 1;
	isGlobal = 0;
	isTriggerActivated = 1;
	isDisposable = 0;
	is3DEN = 0;
	curatorInfoType = "";
	curatorInfoTypeEmpty = "";
	curatorCanAttach = 0;
	icon = "A3\modules_f_beta\data\FiringDrills\iconModuleFD_ca.paa";
	class Attributes: AttributesBase
	{
		class KH_ModuleUnitLoiterRadius: Edit
		{
			displayName = "Enter Buildings";
			tooltip = "Radius, in metres, within which the units will loiter.";
			property = "KH_ModuleUnitLoiterRadius";
			defaultValue = "'100'";
		};
		class KH_ModuleUnitLoiterEnterBuildings: Checkbox
		{
			displayName = "Enter Buildings";
			tooltip = "True allows the units to enter buildings.";
			property = "KH_ModuleUnitLoiterEnterBuildings";
			defaultValue = "true";
		};
		class KH_ModuleUnitLoiterRandomDelayMinimum: Edit
		{
			displayName = "Random Delay Minimum";
			tooltip = "Randomized minimum delay between loiter actions.";
			property = "KH_ModuleUnitLoiterRandomDelayMinimum";
			defaultValue = "'5'";
		};
		class KH_ModuleUnitLoiterRandomDelayMiddle: Edit
		{
			displayName = "Random Delay Middle";
			tooltip = "Randomized middle delay between loiter actions..";
			property = "KH_ModuleUnitLoiterRandomDelayMiddle";
			defaultValue = "'15'";
		};
    	class KH_ModuleUnitLoiterRandomDelayMaximum: Edit
		{
			displayName = "Random Delay Maximum";
			tooltip = "Randomized maximum delay between loiter actions.";
			property = "KH_ModuleUnitLoiterRandomDelayMaximum";
			defaultValue = "'30'";
		};
		class KH_ModuleUnitLoiterCondition: EditCodeMulti5
		{
			displayName = "Condition";
			tooltip = "Unscheduled code, executed locally to the loitering unit, that must return true in order for the unit to loiter. Passed arguments available through _this are: [_unit (OBJECT)].";
			property = "KH_ModuleUnitLoiterCondition";
			defaultValue = "'true'";
		};
		class ModuleDescription: ModuleDescription {};
	};
	class ModuleDescription: ModuleDescription
	{
		description[] = {"Makes the synchronized units loiter around their current area within a radius. Activates multiple times, with a trigger, on the server."};
		sync[] = {"AnyPerson"};
		position = 0;
		direction = 0;
		optional = 0;
		duplicate = 1;
	};
};