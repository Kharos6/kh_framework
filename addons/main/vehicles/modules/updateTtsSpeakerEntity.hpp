class KH_ModuleUpdateTTSSpeakerEntity: Module_F
{
	author = "Kharos";
	scope = 2;
	scopeCurator = 1;
	canSetArea = 0;
	canSetAreaHeight = 0;
	canSetAreaShape = 0;
	displayName = "Update TTS Speaker Entity";
	category = "KH_Modules";
	function = "KH_fnc_moduleUpdateTtsSpeakerEntity";
	functionPriority = 1;
	isGlobal = 0;
	isTriggerActivated = 1;
	isDisposable = 0;
	is3DEN = 0;
	curatorInfoType = "";
	curatorInfoTypeEmpty = "";
	curatorCanAttach = 0;
	icon = "iconmodule";
	class Attributes: AttributesBase
	{
		class KH_ModuleUpdateTTSSpeakerEntityActivateOnce: Checkbox
		{
			displayName = "Activate Once";
			tooltip = "True prevents the module from being able to activate multiple times.";
			property = "KH_ModuleUpdateTTSSpeakerEntityActivateOnce";
			defaultValue = "true";
		};
		class KH_ModuleUpdateTTSSpeakerEntityText: Edit
		{
			displayName = "Text";
			tooltip = "Text that will be transformed into speech.";
			property = "KH_ModuleUpdateTTSSpeakerEntityText";
			defaultValue = "''";
		};
		class KH_ModuleUpdateTTSSpeakerEntityVolume: Edit
		{
			displayName = "Volume";
			tooltip = "Volume.";
			property = "KH_ModuleUpdateTTSSpeakerEntityVolume";
			defaultValue = "'1'";
		};
		class KH_ModuleUpdateTTSSpeakerEntitySpeed: Edit
		{
			displayName = "Volume";
			tooltip = "Speed of the audio playback.";
			property = "KH_ModuleUpdateTTSSpeakerEntitySpeed";
			defaultValue = "'1'";
		};
		class KH_ModuleUpdateTTSSpeakerEntitySpeakerID: Edit
		{
			displayName = "Speaker ID";
			tooltip = "Any speaker ID available to the currently used TTS model.";
			property = "KH_ModuleUpdateTTSSpeakerEntitySpeakerID";
			defaultValue = "'0'";
		};
		class KH_ModuleUpdateTTSSpeakerEntityMaximumDistance: Edit
		{
			displayName = "Maximum Distance";
			tooltip = "Distance beyond which the TTS will not be heard. The volume is linearly interpolated with this value; closer being louder and further being quieter.";
			property = "KH_ModuleUpdateTTSSpeakerEntityMaximumDistance";
			defaultValue = "'100'";
		};
		class KH_ModuleUpdateTTSSpeakerEntityGeneratedFunction: EditCodeMulti5
		{
			displayName = "Generated Function";
			tooltip = "Unscheduled code executed locally to each player when the TTS audio is generated. Passed arguments available through _this are: [_speaker (STRING), _text (STRING)].";
			property = "KH_ModuleUpdateTTSSpeakerEntityGeneratedFunction";
			defaultValue = "''";
		};
		class KH_ModuleUpdateTTSSpeakerEntityFinishedFunction: EditCodeMulti5
		{
			displayName = "Finished Function";
			tooltip = "Unscheduled code executed locally to each player when the TTS audio is finished. Passed arguments available through _this are: [_speaker (STRING), _stopped (BOOL)].";
			property = "KH_ModuleUpdateTTSSpeakerEntityFinishedFunction";
			defaultValue = "''";
		};
		class KH_ModuleUpdateTTSSpeakerEntitySubtitleName: Edit
		{
			displayName = "Subtitle Name";
			tooltip = "Name displayed in the subtitles for the speaker. Leave empty for no subtitles.";
			property = "KH_ModuleUpdateTTSSpeakerEntitySubtitleName";
			defaultValue = "''";
		};
		class KH_ModuleUpdateTTSSpeakerEntityOverride: Checkbox
		{
			displayName = "Override";
			tooltip = "True updates the Volume, Maximum Distance, Generated Function, and Finished Function parameters if TTS audio is already playing for this speaker; it also stops the current audio if the Text parameter is empty.";
			property = "KH_ModuleUpdateTTSSpeakerEntityOverride";
			defaultValue = "false";
		};
		class ModuleDescription: ModuleDescription {};
	};
	class ModuleDescription: ModuleDescription
	{
		description[] = {"Activates text-to-speech on the synchronized entities, playing the audio for every player. Activates multiple times, with a trigger, on the server."};
		sync[] = {"Anything"};
		position = 0;
		direction = 0;
		optional = 0;
		duplicate = 1;
	};
};