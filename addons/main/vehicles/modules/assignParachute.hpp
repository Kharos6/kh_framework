class KH_ModuleAssignParachute: Module_F
{
	scope = 2;
	scopeCurator = 1;
	canSetArea = 0;
	canSetAreaHeight = 0;
	canSetAreaShape = 0;
	displayName = "Assign Paracute";
	category = "KH_Modules";
	function = "KH_fnc_moduleAssignParachute";
	functionPriority = 1;
	isGlobal = 0;
	isTriggerActivated = 1;
	isDisposable = 0;
	is3DEN = 0;
	icon = "\a3\data_f_tank\logos\vanguard_icon_64_ca.paa";
	curatorInfoType = "";
	curatorInfoTypeEmpty = "";
	curatorCanAttach = 0;
	class Attributes: AttributesBase
	{
		class KH_ModuleAssignParachuteSteerable: Checkbox
		{
			displayName = "Steerable";
			tooltip = "<true> provides a steerable parachute. <false> provides a non-steerable parachute. If <Equip> is <false>, the non-steerable parachute is only provided if ace is active; if ace is not active, a steerable parachute is provided.";
			property = "KH_ModuleAssignParachuteSteerable";
			defaultValue = "false";
		};
		class KH_ModuleAssignParachuteEquip: Checkbox
		{
			displayName = "Equip";
			tooltip = "<true> spawns a parachute for each affected unit, with each unit instantly entering their designated parachute. <false> provides a parachute backpack to each affected unit, in which case the unit's original backpack is recovered when the parachute backpack is dropped.";
			property = "KH_ModuleAssignParachuteEquip";
			defaultValue = "false";
		};
		class ModuleDescription: ModuleDescription {};
	};
	class ModuleDescription: ModuleDescription
	{
		description[] = {"Assigns a parachute to the synchronized units. Can be exclusively activated through a trigger or triggers. Can activate multiple times. Activated on the server."};
		sync[] = {"AnyPerson"};
		position = 0;
		direction = 0;
		optional = 0;
		duplicate = 1;
	};
};