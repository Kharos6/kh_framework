class KH_ModuleInitializeTTS: Module_F
{
	author = "Kharos";
	scope = 2;
	scopeCurator = 1;
	canSetArea = 0;
	canSetAreaHeight = 0;
	canSetAreaShape = 0;
	displayName = "Initialize TTS";
	category = "KH_Modules";
	function = "KH_fnc_moduleInitializeTts";
	functionPriority = 1;
	isGlobal = 0;
	isTriggerActivated = 0;
	isDisposable = 1;
	is3DEN = 0;
	curatorInfoType = "";
	curatorInfoTypeEmpty = "";
	curatorCanAttach = 0;
	icon = "iconmodule";
	class Attributes: AttributesBase
	{
		class KH_ModuleInitializeTTSModel: Edit
		{
			displayName = "Model";
			tooltip = "Name of the TTS model folder that will be used. Must be located in the Documents/Arma 3/kh_framework/tts_models folder or any activated mod under the tts_models folder on all player machines. Leave empty to automatically use the first model located within any folder.";
			property = "KH_ModuleInitializeTTSModel";
			defaultValue = "''";
		};
		class KH_ModuleInitializeTTSCPUThreads: Edit
		{
			displayName = "CPU Threads";
			tooltip = "Number of CPU threads used for TTS processing. No more than 4 threads should be used in typical scenarios.";
			property = "KH_ModuleInitializeTTSCPUThreads";
			defaultValue = "'2'";
		};
		class KH_ModuleInitializeTTSNoiseScale: Edit
		{
			displayName = "Noise Scale";
			tooltip = "Controls the frequency of the noise pattern, defining overall randomness and variation in the voice where lower is more robotic and higher is more natural but possibly unstable. May or may not have an effect depending on the model used.";
			property = "KH_ModuleInitializeTTSNoiseScale";
			defaultValue = "'0.667'";
		};
		class KH_ModuleInitializeTTSNoiseScaleW: Edit
		{
			displayName = "Noise Scale W";
			tooltip = "Controls the frequency of the noise pattern, defining variation in how long each phoneme is held. May or may not have an effect depending on the model used.";
			property = "KH_ModuleInitializeTTSNoiseScaleW";
			defaultValue = "'0.8'";
		};
		class KH_ModuleInitializeTTSLengthScale: Edit
		{
			displayName = "Length Scale";
			tooltip = "Controls the overall speed of the voice. May or may not have an effect depending on the model used.";
			property = "KH_ModuleInitializeTTSLengthScale";
			defaultValue = "'1'";
		};
		class KH_ModuleInitializeTTSGeneratedFunction: EditCodeMulti5
		{
			displayName = "Generated Function";
			tooltip = "Unscheduled code executed locally to each player when TTS is generated and audio starts playing. Passed arguments available through _this are: [_speaker (STRING), _text (STRING)].";
			property = "KH_ModuleInitializeTTSGeneratedFunction";
			defaultValue = "''";
		};
		class KH_ModuleInitializeTTSFinishedFunction: EditCodeMulti5
		{
			displayName = "Finished Function";
			tooltip = "Unscheduled code executed locally to each player when TTS audio finishes. Passed arguments available through _this are: [_speaker (STRING), _stopped (BOOL)].";
			property = "KH_ModuleInitializeTTSFinishedFunction";
			defaultValue = "''";
		};
		class ModuleDescription: ModuleDescription {};
	};
	class ModuleDescription: ModuleDescription
	{
		description[] = {"Initializes the text-to-speech system for all players. Activates only once, without a trigger, on the server."};
		sync[] = {};
		position = 0;
		direction = 0;
		optional = 1;
		duplicate = 1;
	};
};