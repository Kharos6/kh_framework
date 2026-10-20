class KH_ModulePostProcessing: Module_F
{
	author = "Kharos";
	scope = 2;
	scopeCurator = 1;
	canSetArea = 0;
	canSetAreaHeight = 0;
	canSetAreaShape = 0;
	displayName = "Post Processing";
	category = "KH_Modules";
	function = "KH_fnc_modulePostProcessing";
	functionPriority = 1;
	isGlobal = 2;
	isTriggerActivated = 1;
	isDisposable = 0;
	is3DEN = 1;
	curatorInfoType = "";
	curatorInfoTypeEmpty = "";
	curatorCanAttach = 0;
	icon = "\a3\Modules_F_Curator\Data\iconPostprocess_ca.paa";
	class Attributes: AttributesBase
	{
		class KH_ModulePostProcessingType: Combo
		{
			displayName = "Type";
			tooltip = "The rendering pass affected by the post processing.";
			property = "KH_ModulePostProcessingType";
			typeName = "STRING";
			defaultValue = "'SCENE'";
			class Values
			{
				class KH_SCENE {name = "SCENE"; value = "SCENE";};
                class KH_UI {name = "UI"; value = "UI";};
                class KH_BOTH {name = "BOTH"; value = "BOTH";};
			};
		};
		class KH_ModulePostProcessingBlend: Combo
		{
			displayName = "Blend";
			tooltip = "Effect blend type.";
			property = "KH_ModulePostProcessingBlend";
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
		class KH_ModulePostProcessingColor: EditXYZ
		{
			displayName = "Color";
			tooltip = "Color of the effect.";
			property = "KH_ModulePostProcessingColor";
			defaultValue = "[1, 1, 1]";
		};
		class KH_ModulePostProcessingEffect: Edit
		{
			displayName = "Effect";
			tooltip = "Either an integrated post processing effect name or a path to a post processing effect.";
			property = "KH_ModulePostProcessingEffect";
			defaultValue = "''";
		};
		class KH_ModulePostProcessingMinimumDistance: Edit
		{
			displayName = "Minimum Distance";
			tooltip = "Minimum distance from the camera beyond which the effect will be present.";
			property = "KH_ModulePostProcessingMinimumDistance";
			defaultValue = "'0'";
		};
		class KH_ModulePostProcessingMaximumDistance: Edit
		{
			displayName = "Maximum Distance";
			tooltip = "Maximum distance from the camera within which the effect will be present.";
			property = "KH_ModulePostProcessingMaximumDistance";
			defaultValue = "'0'";
		};
		class KH_ModulePostProcessingFalloff: Edit
		{
			displayName = "Falloff";
			tooltip = "Falloff radius in metres, useful only if Minimum Distance or Maximum Distance are not 0.";
			property = "KH_ModulePostProcessingFalloff";
			defaultValue = "'10'";
		};
		class KH_ModulePostProcessingParameters: EditMulti5
		{
			displayName = "Parameters";
			tooltip = "Arguments fed to the shader.";
			property = "KH_ModulePostProcessingParameters";
			defaultValue = "''";
		};
		class KH_ModulePostProcessingShow: Checkbox
		{
			displayName = "Show";
			tooltip = "True shows the effect changes in the editor.";
			property = "KH_ModulePostProcessingShow";
			defaultValue = "true";
		};
		class ModuleDescription: ModuleDescription {};
	};
	class ModuleDescription: ModuleDescription
	{
		description[] = {"Manages post processing in the scene. Activates multiple times, with a trigger, on any machine."};
		sync[] = {};
		position = 0;
		direction = 0;
		optional = 1;
		duplicate = 1;
	};
};