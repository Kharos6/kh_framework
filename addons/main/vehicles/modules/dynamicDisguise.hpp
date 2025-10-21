class KH_ModuleDynamicDisguise: Module_F
{
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
	icon = "\a3\ui_f\data\igui\cfg\simpletasks\letters\d_ca.paa";
	class Attributes: AttributesBase
	{
		class BLUFORUniforms: EditMulti5
		{
			displayName = "BLUFOR Uniforms";
			tooltip = "Array of strings of uniform class names designated as BLUFOR.";
			property = "KH_ModuleDynamicDisguiseBluforUniforms";
			defaultValue = "''";
		};
		class OPFORUniforms: EditMulti5
		{
			displayName = "OPFOR Uniforms";
			tooltip = "Array of strings of uniform class names designated as OPFOR.";
			property = "KH_ModuleDynamicDisguiseOpforUniforms";
			defaultValue = "''";
		};
		class GREENFORUniforms: EditMulti5
		{
			displayName = "GREENFOR Uniforms";
			tooltip = "Array of strings of uniform class names designated as GREENFOR.";
			property = "KH_ModuleDynamicDisguiseGreenforUniforms";
			defaultValue = "''";
		};
		class BLUFORVests: EditMulti5
		{
			displayName = "BLUFOR Vests";
			tooltip = "Array of strings of vest class names designated as BLUFOR.";
			property = "KH_ModuleDynamicDisguiseBluforVests";
			defaultValue = "''";
		};
		class OPFORVests: EditMulti5
		{
			displayName = "OPFOR Vests";
			tooltip = "Array of strings of vest class names designated as OPFOR.";
			property = "KH_ModuleDynamicDisguiseOpforVests";
			defaultValue = "''";
		};
		class GREENFORVests: EditMulti5
		{
			displayName = "GREENFOR Vests";
			tooltip = "Array of strings of vest class names designated as GREENFOR.";
			property = "KH_ModuleDynamicDisguiseGreenforVests";
			defaultValue = "''";
		};
		class BLUFORHeadgear: EditMulti5
		{
			displayName = "BLUFOR Headgear";
			tooltip = "Array of strings of headgear class names designated as BLUFOR.";
			property = "KH_ModuleDynamicDisguiseBluforHeadgear";
			defaultValue = "''";
		};
		class OPFORHeadgear: EditMulti5
		{
			displayName = "OPFOR Headgear";
			tooltip = "Array of strings of headgear class names designated as OPFOR.";
			property = "KH_ModuleDynamicDisguiseOpforHeadgear";
			defaultValue = "''";
		};
		class GREENFORHeadgear: EditMulti5
		{
			displayName = "GREENFOR Headgear";
			tooltip = "Array of strings of headgear class names designated as GREENFOR.";
			property = "KH_ModuleDynamicDisguiseGreenforHeadgear";
			defaultValue = "''";
		};
		class Captive: Checkbox
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