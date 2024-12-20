class KH_ModulePlaySound3D: Module_F
{
	scope = 2;
	scopeCurator = 1;
	canSetArea = 0;
	canSetAreaHeight = 0;
	canSetAreaShape = 0;
	displayName = "Play Sound 3D";
	category = "KH_Modules";
	function = "KH_fnc_modulePlaySound3d";
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
		class KH_ModulePlaySound3DSound: Edit
		{
			displayName = "Sound";
			tooltip = "Class name of the sound to play.";
			property = "KH_ModulePlaySound3DSound";
			defaultValue = "''";
		};
		class KH_ModulePlaySound3DDistance: Edit
		{
			displayName = "Distance";
			tooltip = "Distance at which the sound can be heard.";
			property = "KH_ModulePlaySound3DDistance";
			defaultValue = "''";
		};
		class KH_ModulePlaySound3DPitch: Edit
		{
			displayName = "Pitch";
			tooltip = "Pitch of the sound.";
			property = "KH_ModulePlaySound3DPitch";
			defaultValue = "''";
		};
		class KH_ModulePlaySound3DAttenuationType: Combo
		{
			displayName = "Attenuation Type";
			tooltip = "<'NONE'> prevents the sound from being muffled by interiors or vehicles. <'ALLOWED'> allows the sound to be muffled by interiors or vehicles. <'SPEECH'> prevents filters from being applied to the sound, making the sound act like speech.";
			property = "KH_ModulePlaySound3DAttenuationType";
			typeName = "STRING";
			defaultValue = "'2'";
			class Values
			{
				class KH_None {name = "NONE"; value = "2";};
				class KH_Allowed {name = "ALLOWED"; value = "0";};
				class KH_Speech {name = "SPEECH"; value = "1";};
			};
		};
		class KH_ModulePlaySound3DSimulateSpeed: Checkbox
		{
			displayName = "Simulate Speed";
			tooltip = "<true> simulates the speed of sound, making it travel instead of being instantaneous.";
			property = "KH_ModulePlaySound3DSimulateSpeed";
			defaultValue = "false";
		};
		class ModuleDescription: ModuleDescription {};
	};
	class ModuleDescription: ModuleDescription
	{
		description[] = {"Plays 3D sound from the position of this module for every synchronized player. Must be activated through a trigger or triggers. Can activate multiple times. Activated on the server."};
		sync[] = {"AnyPlayer"};
		position = 1;
		direction = 0;
		optional = 0;
		duplicate = 1;
	};
};