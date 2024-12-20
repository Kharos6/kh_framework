class KH_ModulePlayMusic: Module_F
{
	scope = 2;
	scopeCurator = 1;
	canSetArea = 0;
	canSetAreaHeight = 0;
	canSetAreaShape = 0;
	displayName = "Play Music";
	category = "KH_Modules";
	function = "KH_fnc_modulePlayMusic";
	functionPriority = 1;
	isGlobal = 0;
	isTriggerActivated = 1;
	isDisposable = 0;
	is3DEN = 0;
	icon = "\a3\modules_f_curator\data\iconsound_ca.paa";
	curatorInfoType = "";
	curatorInfoTypeEmpty = "";
	curatorCanAttach = 0;
	class Attributes: AttributesBase
	{
		class KH_ModulePlayMusicMusic: Edit
		{
			displayName = "Music";
			tooltip = "Class name of the music to play.";
			property = "KH_ModulePlayMusicMusic";
			defaultValue = "''";
		};
		class KH_ModulePlayMusicStartTime: Edit
		{
			displayName = "Start Time";
			tooltip = "Time from which the music will play, in seconds.";
			property = "KH_ModulePlayMusicStartTime";
			defaultValue = "''";
		};
		class ModuleDescription: ModuleDescription {};
	};
	class ModuleDescription: ModuleDescription
	{
		description[] = {"Plays music for every synchronized player. Must be activated through a trigger or triggers. Can activate multiple times. Activated on the server."};
		sync[] = {"AnyPerson"};
		position = 0;
		direction = 0;
		optional = 0;
		duplicate = 1;
	};
};