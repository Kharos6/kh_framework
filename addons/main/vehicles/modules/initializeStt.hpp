class KH_ModuleInitializeSTT: Module_F
{
	author = "Kharos";
	scope = 2;
	scopeCurator = 1;
	canSetArea = 0;
	canSetAreaHeight = 0;
	canSetAreaShape = 0;
	displayName = "Initialize STT";
	category = "KH_Modules";
	function = "KH_fnc_moduleInitializeStt";
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
		class KH_ModuleInitializeSTTModel: Edit
		{
			displayName = "Model";
			tooltip = "Name of the STT model folder that will be used. Must be located in the Documents/Arma 3/kh_framework/stt_models folder or any activated mod under the stt_models folder on all player machines. Leave empty to automatically use the first model located within any folder.";
			property = "KH_ModuleInitializeSTTModel";
			defaultValue = "''";
		};
		class KH_ModuleInitializeSTTCPUThreads: Edit
		{
			displayName = "CPU Threads";
			tooltip = "Number of CPU threads used for STT processing. No more than 4 threads should be used in typical scenarios.";
			property = "KH_ModuleInitializeSTTCPUThreads";
			defaultValue = "'2'";
		};
		class KH_ModuleInitializeSTTTranscriptionFunction: EditCodeMulti5
		{
			displayName = "Transcription Function";
			tooltip = "Unscheduled code executed locally to each player when their STT recording is transcribed. Passed arguments available through _this are: [_text (STRING)].";
			property = "KH_ModuleInitializeSTTTranscriptionFunction";
			defaultValue = "''";
		};
		class ModuleDescription: ModuleDescription {};
	};
	class ModuleDescription: ModuleDescription
	{
		description[] = {"Initializes the speech-to-text system for all players. Activates only once, without a trigger, on the server."};
		sync[] = {};
		position = 0;
		direction = 0;
		optional = 1;
		duplicate = 1;
	};
};