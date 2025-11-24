class KH_ModuleUpdateAIInstance: Module_F
{
	author = "Kharos";
	scope = 2;
	scopeCurator = 1;
	canSetArea = 0;
	canSetAreaHeight = 0;
	canSetAreaShape = 0;
	displayName = "Update AI Instance";
	category = "KH_Modules";
	function = "KH_fnc_moduleUpdateAiInstance";
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
		class KH_ModuleUpdateAIInstanceActivateOnce: Checkbox
		{
			displayName = "Activate Once";
			tooltip = "True prevents the module from being able to activate multiple times.";
			property = "KH_ModuleUpdateAIInstanceActivateOnce";
			defaultValue = "true";
		};
		class KH_ModuleUpdateAIInstanceOwner: Edit
		{
			displayName = "Owner";
			tooltip = "The actual machine on which the AI instance will be updated; can be a variable name of a playable unit or just SERVER.";
			property = "KH_ModuleUpdateAIInstanceOwner";
			defaultValue = "'SERVER'";
		};
		class KH_ModuleUpdateAIInstanceName: Edit
		{
			displayName = "Name";
			tooltip = "Name of the AI that will be updated.";
			property = "KH_ModuleUpdateAIInstanceName";
			defaultValue = "''";
		};
		class KH_ModuleUpdateAIInstanceSystemPrompt: EditCodeMulti5
		{
			displayName = "System Prompt";
			tooltip = "Unscheduled code, executed locally to the owner, that must return a string or structured text which will be fed to the system prompt; the string or structured text should contain instructions that the AI will strictly adhere to. Leave empty for no change. Passed arguments available through _this are: [_name (STRING)].";
			property = "KH_ModuleUpdateAIInstanceSystemPrompt";
			defaultValue = "''";
		};
		class KH_ModuleUpdateAIInstanceUserPrompt: EditCodeMulti5
		{
			displayName = "User Prompt";
			tooltip = "Unscheduled code, executed locally to the owner, that must return a string or structured text which will be fed to the user prompt; the string or structured text should contain instructions that the AI will fulfill. Leave empty for no change. Passed arguments available through _this are: [_name (STRING)].";
			property = "KH_ModuleUpdateAIInstanceUserPrompt";
			defaultValue = "''";
		};
		class KH_ModuleUpdateAIInstanceResponseProgressFunction: EditCodeMulti5
		{
			displayName = "Response Progress Function";
			tooltip = "Unscheduled code executed locally to the owner on the next response. Passed arguments available through _this are: [_name (STRING), _response (STRING)].";
			property = "KH_ModuleUpdateAIInstanceResponseProgressFunction";
			defaultValue = "''";
		};
		class KH_ModuleUpdateAIInstanceResponseFunction: EditCodeMulti5
		{
			displayName = "Response Function";
			tooltip = "Unscheduled code executed locally to the owner on the next response. Passed arguments available through _this are: [_name (STRING), _response (STRING)].";
			property = "KH_ModuleUpdateAIInstanceResponseFunction";
			defaultValue = "''";
		};
		class KH_ModuleUpdateAIInstanceTriggerInference: Checkbox
		{
			displayName = "Trigger Inference";
			tooltip = "True triggers an inference, making the AI respond. If the AI is currently generating a response, the inference will trigger when the response is complete.";
			property = "KH_ModuleUpdateAIInstanceTriggerInference";
			defaultValue = "true";
		};
		class KH_ModuleUpdateAIInstanceAbortGeneration: Checkbox
		{
			displayName = "Abort Generation";
			tooltip = "True aborts the current generation if any is in progress.";
			property = "KH_ModuleUpdateAIInstanceAbortGeneration";
			defaultValue = "true";
		};
		class KH_ModuleUpdateAIInstanceResetContext: Checkbox
		{
			displayName = "Reset Context";
			tooltip = "True clears the conversation history while retaining the system prompt.";
			property = "KH_ModuleUpdateAIInstanceResetContext";
			defaultValue = "false";
		};
		class KH_ModuleUpdateAIInstanceLogGeneration: Checkbox
		{
			displayName = "Log Generation";
			tooltip = "True enables logging for token generation.";
			property = "KH_ModuleUpdateAIInstanceLogGeneration";
			defaultValue = "false";
		};
		class KH_ModuleUpdateAIInstanceStop: Checkbox
		{
			displayName = "Stop";
			tooltip = "True terminates the AI instance.";
			property = "KH_ModuleUpdateAIInstanceStop";
			defaultValue = "false";
		};
		class ModuleDescription: ModuleDescription {};
	};
	class ModuleDescription: ModuleDescription
	{
		description[] = {"Updates the state of an AI instance, with each subsequent update overriding the previous one. Activates multiple times, with a trigger, on the server."};
		sync[] = {};
		position = 0;
		direction = 0;
		optional = 1;
		duplicate = 1;
	};
};