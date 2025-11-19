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
		class KH_ModuleUpdateAIInstanceSystemPrompt: EditMulti5
		{
			displayName = "System Prompt";
			tooltip = "Instructions that the AI will strictly adhere to. Leave empty for no change.";
			property = "KH_ModuleUpdateAIInstanceSystemPrompt";
			defaultValue = "''";
		};
		class KH_ModuleUpdateAIInstanceUserPrompt: EditMulti5
		{
			displayName = "User Prompt";
			tooltip = "Instructions that the AI will fulfill. Leave empty for no change.";
			property = "KH_ModuleUpdateAIInstanceUserPrompt";
			defaultValue = "''";
		};
		class KH_ModuleUpdateAIInstanceTriggerInference: Checkbox
		{
			displayName = "Trigger Inference";
			tooltip = "Triggers an inference, making the AI respond.";
			property = "KH_ModuleUpdateAIInstanceTriggerInference";
			defaultValue = "true";
		};
		class ModuleDescription: ModuleDescription {};
	};
	class ModuleDescription: ModuleDescription
	{
		description[] = {"Updates the state of an AI instance. Activates multiple times, without a trigger, on the server."};
		sync[] = {};
		position = 0;
		direction = 0;
		optional = 1;
		duplicate = 1;
	};
};