class KH_ModuleDisplaySubtitle: Module_F
{
	scope = 2;
	scopeCurator = 1;
	canSetArea = 0;
	canSetAreaHeight = 0;
	canSetAreaShape = 0;
	displayName = "Display Subtitle";
	category = "KH_Modules";
	function = "KH_fnc_moduleDisplaySubtitle";
	functionPriority = 1;
	isGlobal = 0;
	isTriggerActivated = 1;
	isDisposable = 0;
	is3DEN = 0;
	icon = "\a3\modules_f\data\icontasksetdescription_ca.paa";
	curatorInfoType = "";
	curatorInfoTypeEmpty = "";
	curatorCanAttach = 0;
	class Attributes: AttributesBase
	{
		class KH_ModuleDisplaySubtitleSpeaker: Edit
		{
			displayName = "Speaker";
			tooltip = "Name of the person speaking in the subtitle.";
			property = "KH_ModuleDisplaySubtitleSpeaker";
			defaultValue = "''";
		};
		class KH_ModuleDisplaySubtitleText: Edit
		{
			displayName = "Text";
			tooltip = "Subtitle Text.";
			property = "KH_ModuleDisplaySubtitleText";
			defaultValue = "''";
		};
		class KH_ModuleDisplaySubtitleDuration: Edit
		{
			displayName = "Duration";
			tooltip = "Duration of the subtitle.";
			property = "KH_ModuleDisplaySubtitleDuration";
			defaultValue = "''";
		};
		class ModuleDescription: ModuleDescription {};
	};
	class ModuleDescription: ModuleDescription
	{
		description[] = {"Displays subtitles for every synchronized player. Can be exclusively activated through a trigger or triggers. Can activate multiple times. Activated on the server."};
		sync[] = {"AnyPerson"};
		position = 0;
		direction = 0;
		optional = 0;
		duplicate = 1;
	};
};