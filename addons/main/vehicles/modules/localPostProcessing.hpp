class KH_ModuleLocalPostProcessing: Module_F
{
	author = "Kharos";
	scope = 2;
	scopeCurator = 1;
	canSetArea = 1;
	canSetAreaHeight = 1;
	canSetAreaShape = 1;
	displayName = "Local Post Processing";
	category = "KH_Modules";
	function = "KH_fnc_moduleLocalPostProcessing";
	functionPriority = 1;
	isGlobal = 2;
	isTriggerActivated = 1;
	isDisposable = 0;
	is3DEN = 1;
	curatorInfoType = "";
	curatorInfoTypeEmpty = "";
	curatorCanAttach = 0;
	icon = "\a3\Modules_F_Curator\Data\iconPostprocess_ca.paa";
    class AttributeValues
    {
        size3[] = {100, 100, 100};
        isRectangle = 0;
    };
	class Attributes: AttributesBase
	{
		class KH_ModuleLocalPostProcessingBlend: Combo
		{
			displayName = "Blend";
			tooltip = "Effect blend type.";
			property = "KH_ModuleLocalPostProcessingBlend";
			typeName = "STRING";
			defaultValue = "'NORMAL'";
			class Values
			{
				class KH_NORMAL {name = "NORMAL"; value = "NORMAL";};
				class KH_ADDITIVE {name = "ADDITIVE"; value = "ADDITIVE";};
				class KH_MULTIPLY {name = "MULTIPLY"; value = "MULTIPLY";};
				class KH_SCREEN {name = "SCREEN"; value = "SCREEN";};
				class KH_LIGHTEN {name = "LIGHTEN"; value = "LIGHTEN";};
				class KH_DARKEN {name = "DARKEN"; value = "DARKEN";};
			};
		};
		class KH_ModuleLocalPostProcessingColor: EditXYZ
		{
			displayName = "Color";
			tooltip = "Color of the effect.";
			property = "KH_ModuleLocalPostProcessingColor";
			defaultValue = "[1, 1, 1]";
		};
		class KH_ModuleLocalPostProcessingEffect: Edit
		{
			displayName = "Effect";
			tooltip = "Either an integrated post processing effect name or a path to a post processing effect.";
			property = "KH_ModuleLocalPostProcessingEffect";
			defaultValue = "''";
		};
		class KH_ModuleLocalPostProcessingFalloff: Edit
		{
			displayName = "Falloff";
			tooltip = "Falloff radius in metres.";
			property = "KH_ModuleLocalPostProcessingFalloff";
			defaultValue = "'0'";
		};
		class KH_ModuleLocalPostProcessingInverse: Checkbox
		{
			displayName = "Inverse";
			tooltip = "True shows the effect everywhere except the affected area or radius.";
			property = "KH_ModuleLocalPostProcessingInverse";
			defaultValue = "false";
		};
		class KH_ModuleLocalPostProcessingParameters: EditMulti5
		{
			displayName = "Parameters";
			tooltip = "Arguments fed to the shader.";
			property = "KH_ModuleLocalPostProcessingParameters";
			defaultValue = "''";
		};
		class KH_ModuleLocalPostProcessingShow: Checkbox
		{
			displayName = "Show";
			tooltip = "True shows the effect changes in the editor.";
			property = "KH_ModuleLocalPostProcessingShow";
			defaultValue = "true";
		};
		class ModuleDescription: ModuleDescription {};
	};
	class ModuleDescription: ModuleDescription
	{
		description[] = {"Manages local post processing in the scene. Activates multiple times, with a trigger, on any machine."};
		sync[] = {};
		position = 1;
		direction = 1;
		optional = 1;
		duplicate = 1;
	};
};