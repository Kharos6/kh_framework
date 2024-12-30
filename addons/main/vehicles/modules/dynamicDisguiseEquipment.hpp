class KH_ModuleDynamicDisguiseEquipment: Module_F
{
	scope = 2;
	scopeCurator = 1;
	canSetArea = 0;
	canSetAreaHeight = 0;
	canSetAreaShape = 0;
	displayName = "Dynamic Disguise Equipment";
	category = "KH_Modules";
	function = "KH_fnc_moduleDynamicDisguiseEquipment";
	functionPriority = 1;
	isGlobal = 0;
	isTriggerActivated = 0;
	isDisposable = 1;
	is3DEN = 0;
	icon = "\a3\ui_f\data\igui\cfg\simpletasks\letters\d_ca.paa";
	curatorInfoType = "";
	curatorInfoTypeEmpty = "";
	curatorCanAttach = 0;
	class Attributes: AttributesBase
	{
		class KH_ModuleDynamicDisguiseEquipmentSide: Combo
		{
			displayName = "Side";
			tooltip = "Side with which the equipment of this unit will be affiliated.";
			property = "KH_ModuleDynamicDisguiseEquipmentSide";
			typeName = "STRING";
			defaultValue = "'BLUFOR'";
			class Values
			{
				class KH_West {name = "BLUFOR"; value = "BLUFOR";};
				class KH_East {name = "OPFOR"; value = "OPFOR";};
				class KH_Resistance {name = "GREENFOR"; value = "GREENFOR";};
			};
		};
		class KH_ModuleDynamicDisguiseEquipmentUniforms: Checkbox
		{
			displayName = "Uniforms";
			tooltip = "<true> inserts the uniforms of synchronized units to the desired side's uniform table.";
			property = "KH_ModuleDynamicDisguiseEquipmentUniforms";
			defaultValue = "false";
		};
		class KH_ModuleDynamicDisguiseEquipmentVests: Checkbox
		{
			displayName = "Vests";
			tooltip = "<true> inserts the vests of synchronized units to the desired side's uniform table.";
			property = "KH_ModuleDynamicDisguiseEquipmentVests";
			defaultValue = "false";
		};
		class KH_ModuleDynamicDisguiseEquipmentHeadgear: Checkbox
		{
			displayName = "Headgear";
			tooltip = "<true> inserts the headgear of synchronized units to the desired side's uniform table.";
			property = "KH_ModuleDynamicDisguiseEquipmentHeadgear";
			defaultValue = "false";
		};
		class ModuleDescription: ModuleDescription {};
	};
	class ModuleDescription: ModuleDescription
	{
		description[] = {"Takes the uniform or headgear, or both, from the synchronized units, and assigns them as affiliated with the desired side for the Dynamic Disguise system. Will work in conjunction with the Dynamic Disguise arrays from the Eden attribute. Avoid using on players for consistency reasons. Activates without a trigger. Activated on the server."};
		sync[] = {"AnyPerson"};
		position = 0;
		direction = 0;
		optional = 0;
		duplicate = 1;
	};
};