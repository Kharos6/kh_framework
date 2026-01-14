class KH_ModulePlayRecordedMission: Module_F
{
	author = "Kharos";
	scope = 2;
	scopeCurator = 1;
	canSetArea = 0;
	canSetAreaHeight = 0;
	canSetAreaShape = 0;
	displayName = "Play Recorded Mission";
	category = "KH_Modules";
	function = "KH_fnc_modulePlayRecordedMission";
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
		class KH_ModulePlayRecordedMissionIdentifier: Edit
		{
			displayName = "Identifier";
			tooltip = "Identifier of a mission recording.";
			property = "KH_ModulePlayRecordedMissionIdentifier";
			defaultValue = "''";
		};
		class ModuleDescription: ModuleDescription {};
	};
	class ModuleDescription: ModuleDescription
	{
		description[] = {"Replays a mission that was recorded. Activates only once, without a trigger, on the server."};
		sync[] = {};
		position = 0;
		direction = 0;
		optional = 1;
		duplicate = 1;
	};
};