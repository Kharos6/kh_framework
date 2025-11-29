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
		class KH_ModuleUpdateTTSSpeakerEntityText: EditCodeMulti5
		{
			displayName = "Text";
			tooltip = "Unscheduled code, executed locally to each player, that must return a string or structured text which will be fed to the TTS engine.";
			property = "KH_ModuleUpdateTTSSpeakerEntityText";
			defaultValue = "''";
		};
		class KH_ModuleUpdateTTSSpeakerEntityVolume: Edit
		{
			displayName = "Volume";
			tooltip = "Volume of the audio playback.";
			property = "KH_ModuleUpdateTTSSpeakerEntityVolume";
			defaultValue = "'1'";
		};
		class KH_ModuleUpdateTTSSpeakerEntitySpeed: Edit
		{
			displayName = "Speed";
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
		class KH_ModuleUpdateTTSSpeakerEntityEffectChain: EditCodeMulti5
		{
			displayName = "Effect Chain";
			tooltip = "Unscheduled code, executed locally to each player, that must return an array of arrays where the first element is a string representing the name of an effect and the second element is a scalar representing the effect magnitude. Possible effects are: ['lowpass', 0-1], ['highpass', 0-1], ['distortion', 0-1], ['bitcrush', 0-1], ['noise', 0-1], ['ring_mod', 0-1000], ['pitch_shift', 0-2], ['echo_delay', 0-1], ['echo_decay', 0-0.95], and ['reverb', 0-1]. Almost any effect can be used multiple times and is applied in order of addition. The pitch_shift and echo_decay are the only effects that cannot be applied multiple times.";
			property = "KH_ModuleUpdateTTSSpeakerEntityEffectChain";
			defaultValue = "'[]'";
		};
		class KH_ModuleUpdateTTSSpeakerEntityGeneratedFunction: EditCodeMulti5
		{
			displayName = "Generated Function";
			tooltip = "Unscheduled code executed locally to each player when the TTS audio is generated. Passed arguments available through _this are: [_speaker (STRING), _text (STRING), _duration (SCALAR)].";
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
			tooltip = "True interrupts the current audio to play new audio. Can be used with an empty string from the Text parameter to simply stop the current audio.";
			property = "KH_ModuleUpdateTTSSpeakerEntityOverride";
			defaultValue = "true";
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