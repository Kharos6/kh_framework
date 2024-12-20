class KH_ModulePlaySound2D: Module_F
{
	scope = 2;
	scopeCurator = 1;
	canSetArea = 0;
	canSetAreaHeight = 0;
	canSetAreaShape = 0;
	displayName = "Play Sound 2D";
	category = "KH_Modules";
	function = "KH_fnc_modulePlaySound2d";
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
		class KH_ModulePlaySound2DSound: Edit
		{
			displayName = "Sound";
			tooltip = "Class name of the sound to play.";
			property = "KH_ModulePlaySound2DSound";
			defaultValue = "''";
		};
		class KH_ModulePlaySound2DAttenuationType: Combo
		{
			displayName = "Attenuation Type";
			tooltip = "<'NONE'> prevents the sound from being muffled by interiors or vehicles. <'ALLOWED'> allows the sound to be muffled by interiors or vehicles. <'SPEECH'> prevents filters from being applied to the sound, making the sound act like speech.";
			property = "KH_ModulePlaySound2DAttenuationType";
			defaultValue = "2";
			class Values
			{
				class KH_None {name = "NONE"; value = 2;};
				class KH_Allowed {name = "ALLOWED"; value = 0;};
				class KH_Speech {name = "SPEECH"; value = 1;};
			};
		};
		class KH_ModulePlaySound2DStartTime: Edit
		{
			displayName = "Start Time";
			tooltip = "Time from which the sound will play, in seconds.";
			property = "KH_ModulePlaySound2DStartTime";
			defaultValue = "''";
		};
		class ModuleDescription: ModuleDescription {};
	};
	class ModuleDescription: ModuleDescription
	{
		description[] = {"Plays 2D sound for every synchronized player. Must be activated through a trigger or triggers. Can activate multiple times. Activated on the server."};
		sync[] = {"AnyPlayer"};
		position = 0;
		direction = 0;
		optional = 0;
		duplicate = 1;
	};
};