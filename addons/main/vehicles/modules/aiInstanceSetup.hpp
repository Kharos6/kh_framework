class KH_ModuleAIInstanceSetup: Module_F
{
	author = "Kharos";
	scope = 2;
	scopeCurator = 1;
	canSetArea = 0;
	canSetAreaHeight = 0;
	canSetAreaShape = 0;
	displayName = "AI Instance Setup";
	category = "KH_Modules";
	function = "KH_fnc_moduleAiInstanceSetup";
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
		class KH_ModuleAIInstanceSetupOwner: Edit
		{
			displayName = "Owner";
			tooltip = "The actual machine that will host the AI instance; can be a variable name of a playable unit or just SERVER.";
			property = "KH_ModuleAIInstanceSetupOwner";
			defaultValue = "'SERVER'";
		};
		class KH_ModuleAIInstanceSetupName: Edit
		{
			displayName = "Name";
			tooltip = "Name of the AI that will act as its identifier.";
			property = "KH_ModuleAIInstanceSetupName";
			defaultValue = "''";
		};
		class KH_ModuleAIInstanceSetupModel: Edit
		{
			displayName = "Model";
			tooltip = "Name of the AI model that will be used. Must include the .gguf extension and be located in Documents/Arma 3/kh_framework/ai_models on the machine hosting the AI instance.";
			property = "KH_ModuleAIInstanceSetupModel";
			defaultValue = "''";
		};
		class KH_ModuleAIInstanceSetupMarkerSystemStart: Edit
		{
			displayName = "Marker: System Start";
			tooltip = "Symbol sequence used to define the start of the system sequence; dependant on the model.";
			property = "KH_ModuleAIInstanceSetupMarkerSystemStart";
			defaultValue = "'<|begin_of_text|><|start_header_id|>system<|end_header_id|>'";
		};
		class KH_ModuleAIInstanceSetupMarkerSystemEnd: Edit
		{
			displayName = "Marker: System End";
			tooltip = "Symbol sequence used to define the end of the system sequence; dependant on the model.";
			property = "KH_ModuleAIInstanceSetupMarkerSystemEnd";
			defaultValue = "'<|eot_id|>'";
		};
		class KH_ModuleAIInstanceSetupMarkerUserStart: Edit
		{
			displayName = "Marker: User Start";
			tooltip = "Symbol sequence used to define the start of the user sequence; dependant on the model.";
			property = "KH_ModuleAIInstanceSetupMarkerUserStart";
			defaultValue = "'<|start_header_id|>user<|end_header_id|>'";
		};
		class KH_ModuleAIInstanceSetupMarkerUserEnd: Edit
		{
			displayName = "Marker: User End";
			tooltip = "Symbol sequence used to define the end of the user sequence; dependant on the model.";
			property = "KH_ModuleAIInstanceSetupMarkerUserEnd";
			defaultValue = "'<|eot_id|>'";
		};
		class KH_ModuleAIInstanceSetupMarkerAssistantStart: Edit
		{
			displayName = "Marker: Assistant Start";
			tooltip = "Symbol sequence used to define the start of the assistant sequence; dependant on the model.";
			property = "KH_ModuleAIInstanceSetupMarkerAssistantStart";
			defaultValue = "'<|start_header_id|>assistant<|end_header_id|>'";
		};
		class KH_ModuleAIInstanceSetupMarkerAssistantEnd: Edit
		{
			displayName = "Marker: Assistant End";
			tooltip = "Symbol sequence used to define the end of the assistant sequence; dependant on the model.";
			property = "KH_ModuleAIInstanceSetupMarkerAssistantEnd";
			defaultValue = "'<|eot_id|>'";
		};
		class KH_ModuleAIInstanceSetupSystemPrompt: EditMulti5
		{
			displayName = "System Prompt";
			tooltip = "Base instructions that the AI will strictly adhere to.";
			property = "KH_ModuleAIInstanceSetupSystemPrompt";
			defaultValue = "''";
		};
		class KH_ModuleAIInstanceSetupUserPrompt: EditMulti5
		{
			displayName = "User Prompt";
			tooltip = "Default instructions that the AI will fulfill.";
			property = "KH_ModuleAIInstanceSetupUserPrompt";
			defaultValue = "''";
		};
		class KH_ModuleAIInstanceSetupContextSize: Edit
		{
			displayName = "Context Size";
			tooltip = "Number of tokens available to the entire system.";
			property = "KH_ModuleAIInstanceSetupContextSize";
			defaultValue = "'32768'";
		};
		class KH_ModuleAIInstanceSetupMaximumGeneratedTokens: Edit
		{
			displayName = "Maximum Generated Tokens";
			tooltip = "Maximum number of tokens that the AI can generate.";
			property = "KH_ModuleAIInstanceSetupMaximumGeneratedTokens";
			defaultValue = "'3072'";
		};
		class KH_ModuleAIInstanceSetupTemperature: Edit
		{
			displayName = "Temperature";
			tooltip = "Contorls creativity and randomness in responses; higher values produce more creative and varied outputs while lower values produce focused and consistent outputs. Must be a value between 0 and 2.";
			property = "KH_ModuleAIInstanceSetupTemperature";
			defaultValue = "'0.3'";
		};
		class KH_ModuleAIInstanceSetupTopK: Edit
		{
			displayName = "Top K";
			tooltip = "Limits token selection to the K most probable options; lower values make output more predictable while higher values increase diversity. Must be a value between 0 and 1000.";
			property = "KH_ModuleAIInstanceSetupTopK";
			defaultValue = "'30'";
		};
		class KH_ModuleAIInstanceSetupTopP: Edit
		{
			displayName = "Top K";
			tooltip = "Nucleus sampling threshold that selects tokens until cumulative probability reaches P; lower values are more focused while higher value allow more variety. Must be a value between 0 and 1.";
			property = "KH_ModuleAIInstanceSetupTopP";
			defaultValue = "'0.9'";
		};
		class KH_ModuleAIInstanceSetupBatchSize: Edit
		{
			displayName = "Batch Size";
			tooltip = "Number of tokens processed together in one forward pass; high values can improve speed but use more memory.";
			property = "KH_ModuleAIInstanceSetupBatchSize";
			defaultValue = "'2048'";
		};
		class KH_ModuleAIInstanceSetupMicroBatchSize: Edit
		{
			displayName = "Micro Batch Size";
			tooltip = "Number of tokens physically processed at once by hardware, splitting batches into smaller chunks. Should be half or equal to N_BATCH for optimal performance.";
			property = "KH_ModuleAIInstanceSetupMicroBatchSize";
			defaultValue = "'1024'";
		};
		class KH_ModuleAIInstanceSetupCPUThreads: Edit
		{
			displayName = "CPU Threads";
			tooltip = "Number of CPU threads used during token generation. 4 to 8 threads usually provides optimal performance.";
			property = "KH_ModuleAIInstanceSetupCPUThreads";
			defaultValue = "'4'";
		};
		class KH_ModuleAIInstanceSetupCPUThreadsBatch: Edit
		{
			displayName = "CPU Threads Batch";
			tooltip = "Number of CPU threads used for processing the prompt. Can be higher than the CPU Threads attribute for faster prompt processing without affecting generation speed.";
			property = "KH_ModuleAIInstanceSetupCPUThreadsBatch";
			defaultValue = "'8'";
		};
		class KH_ModuleAIInstanceSetupGPULayers: Edit
		{
			displayName = "GPU Layers";
			tooltip = "Number of model layers offloaded to the GPU. Higher values use more VRAM, but significantly improve speed; a value of 0 means only the CPU is used, which is extremely slow but compatible, while a value of 999 offloads all layers.";
			property = "KH_ModuleAIInstanceSetupGPULayers";
			defaultValue = "'999'";
		};
		class KH_ModuleAIInstanceSetupFlashAttention: Checkbox
		{
			displayName = "Flash Attention";
			tooltip = "Enables an optimized algorithm for faster attention computation on the GPU; only works if GPU acceleration is active. Provides a decent increase in speed with lower memory usage.";
			property = "KH_ModuleAIInstanceSetupFlashAttention";
			defaultValue = "true";
		};
		class KH_ModuleAIInstanceSetupOffloadKVCache: Checkbox
		{
			displayName = "Offload KV Cache";
			tooltip = "Stores the attention cache in VRAM instead of RAM. While faster, it consumes a bit more memory, depending on context length.";
			property = "KH_ModuleAIInstanceSetupOffloadKVCache";
			defaultValue = "true";
		};
		class ModuleDescription: ModuleDescription {};
	};
	class ModuleDescription: ModuleDescription
	{
		description[] = {"Initializes and defines base parameters for an AI instance. Activates only once, without a trigger, on the server."};
		sync[] = {};
		position = 0;
		direction = 0;
		optional = 1;
		duplicate = 1;
	};
};