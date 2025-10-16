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
	class Attributes: AttributesBase
	{
		class Side: Combo
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
		class Uniforms: Checkbox
		{
			displayName = "Uniforms";
			tooltip = "True inserts the uniforms of synchronized units to the desired side's uniform table.";
			property = "KH_ModuleDynamicDisguiseEquipmentUniforms";
			defaultValue = "false";
		};
		class Vests: Checkbox
		{
			displayName = "Vests";
			tooltip = "True inserts the vests of synchronized units to the desired side's uniform table.";
			property = "KH_ModuleDynamicDisguiseEquipmentVests";
			defaultValue = "false";
		};
		class Headgear: Checkbox
		{
			displayName = "Headgear";
			tooltip = "True inserts the headgear of synchronized units to the desired side's uniform table.";
			property = "KH_ModuleDynamicDisguiseEquipmentHeadgear";
			defaultValue = "false";
		};
		class ModuleDescription: ModuleDescription {};
	};
	class ModuleDescription: ModuleDescription
	{
		description[] = {"Takes the uniform, vest, and/or headgear from the synchronized units and assigns them as affiliated with the desired side for the Dynamic Disguise system. Will work in conjunction with the Dynamic Disguise arrays from the Eden attribute. Avoid using on playable units or players. Activates only once, without a trigger, on the server."};
		sync[] = {"AnyPerson"};
		position = 0;
		direction = 0;
		optional = 0;
		duplicate = 1;
	};
};