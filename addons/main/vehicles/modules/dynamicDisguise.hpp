class KH_ModuleDynamicDisguise: Module_F
{
	author = "Kharos";
	scope = 2;
	scopeCurator = 1;
	canSetArea = 0;
	canSetAreaHeight = 0;
	canSetAreaShape = 0;
	displayName = "Dynamic Disguise";
	category = "KH_Modules";
	function = "KH_fnc_moduleDynamicDisguise";
	functionPriority = 1;
	isGlobal = 0;
	isTriggerActivated = 0;
	isDisposable = 1;
	is3DEN = 0;
	curatorInfoType = "";
	curatorInfoTypeEmpty = "";
	curatorCanAttach = 0;
	icon = "\a3\ui_f\data\igui\cfg\simpletasks\letters\d_ca.paa";
	class Attributes: AttributesBase
	{
		class KH_ModuleDynamicDisguiseBLUFORUniforms: EditMulti5
		{
			displayName = "BLUFOR Uniforms";
			tooltip = "Array of strings of uniform class names designated as BLUFOR.";
			property = "KH_ModuleDynamicDisguiseBLUFORUniforms";
			defaultValue = "''";
		};
		class KH_ModuleDynamicDisguiseOPFORUniforms: EditMulti5
		{
			displayName = "OPFOR Uniforms";
			tooltip = "Array of strings of uniform class names designated as OPFOR.";
			property = "KH_ModuleDynamicDisguiseOPFORUniforms";
			defaultValue = "''";
		};
		class KH_ModuleDynamicDisguiseGREENFORUniforms: EditMulti5
		{
			displayName = "GREENFOR Uniforms";
			tooltip = "Array of strings of uniform class names designated as GREENFOR.";
			property = "KH_ModuleDynamicDisguiseGREENFORUniforms";
			defaultValue = "''";
		};
		class KH_ModuleDynamicDisguiseBLUFORVests: EditMulti5
		{
			displayName = "BLUFOR Vests";
			tooltip = "Array of strings of vest class names designated as BLUFOR.";
			property = "KH_ModuleDynamicDisguiseBLUFORVests";
			defaultValue = "''";
		};
		class KH_ModuleDynamicDisguiseOPFORVests: EditMulti5
		{
			displayName = "OPFOR Vests";
			tooltip = "Array of strings of vest class names designated as OPFOR.";
			property = "KH_ModuleDynamicDisguiseOPFORVests";
			defaultValue = "''";
		};
		class KH_ModuleDynamicDisguiseGREENFORVests: EditMulti5
		{
			displayName = "GREENFOR Vests";
			tooltip = "Array of strings of vest class names designated as GREENFOR.";
			property = "KH_ModuleDynamicDisguiseGREENFORVests";
			defaultValue = "''";
		};
		class KH_ModuleDynamicDisguiseBLUFORHeadgear: EditMulti5
		{
			displayName = "BLUFOR Headgear";
			tooltip = "Array of strings of headgear class names designated as BLUFOR.";
			property = "KH_ModuleDynamicDisguiseBLUFORHeadgear";
			defaultValue = "''";
		};
		class KH_ModuleDynamicDisguiseOPFORHeadgear: EditMulti5
		{
			displayName = "OPFOR Headgear";
			tooltip = "Array of strings of headgear class names designated as OPFOR.";
			property = "KH_ModuleDynamicDisguiseOPFORHeadgear";
			defaultValue = "''";
		};
		class KH_ModuleDynamicDisguiseGREENFORHeadgear: EditMulti5
		{
			displayName = "GREENFOR Headgear";
			tooltip = "Array of strings of headgear class names designated as GREENFOR.";
			property = "KH_ModuleDynamicDisguiseGREENFORHeadgear";
			defaultValue = "''";
		};
		class KH_ModuleDynamicDisguiseCaptive: Checkbox
		{
			displayName = "Captive";
			tooltip = "True sets disguised units as captives in order to prevent them from being engaged by their original side.";
			property = "KH_ModuleDynamicDisguiseCaptive";
			defaultValue = "false";
		};
		class ModuleDescription: ModuleDescription {};
	};
	class ModuleDescription: ModuleDescription
	{
		description[] = {"Activates a disguise system that dictates the side affiliation of players based on the uniform, vest, and headgear they are wearing. Will work in conjunction with the Dynamic Disguise Equipment modules. Avoid synchronizing to playable units or players due to inconsistent availability. Activates only once, without a trigger, on the server."};
		sync[] = {};
		position = 0;
		direction = 0;
		optional = 1;
		duplicate = 0;
	};
};