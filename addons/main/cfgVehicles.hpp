class CfgVehicles
{
	class Module_F
	{
		class AttributesBase
		{
			class Checkbox;
			class CheckboxNumber;
			class Combo;
			class Default;
			class Edit;
			class ModuleDescription;
			class Units;
		};
		class ModuleDescription;
	};
	class Land_HelipadEmpty_F;
	class KH_HelperArrow: Land_HelipadEmpty_F
	{
		displayName = "Helper (Arrow)";
		editorCategory = "KH_Helpers";
		editorSubcategory = "KH_Invisible";
		vehicleClass = "KH_Helpers";
		icon = "\x\kh\addons\main\ui\helper_arrow_up.paa";
	};	
	class KH_HelperSquare: Land_HelipadEmpty_F
	{
		displayName = "Helper (Square)";
		editorCategory = "KH_Helpers";
		editorSubcategory = "KH_Invisible";
		vehicleClass = "KH_Helpers";
	};
};