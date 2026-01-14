class KH_ModuleRecordMission: Module_F
{
	author = "Kharos";
	scope = 2;
	scopeCurator = 1;
	canSetArea = 0;
	canSetAreaHeight = 0;
	canSetAreaShape = 0;
	displayName = "Record Mission";
	category = "KH_Modules";
	function = "KH_fnc_moduleRecordMission";
	functionPriority = 1;
	isGlobal = 0;
	isTriggerActivated = 1;
	isDisposable = 1;
	is3DEN = 0;
	curatorInfoType = "";
	curatorInfoTypeEmpty = "";
	curatorCanAttach = 0;
	icon = "iconmodule";
	class Attributes: AttributesBase
	{
		class KH_ModuleRecordMissionCollector: Edit
		{
			displayName = "Collector";
			tooltip = "Variable name of the unit whose machine will capture the data. Leave empty to capture on the server. Headless client is recommended due to the CPU intensity of the capture.";
			property = "KH_ModuleRecordMissionCollector";
			defaultValue = "''";
		};
		class KH_ModuleRecordMissionIdentifier: Edit
		{
			displayName = "Identifier";
			tooltip = "Identifier by which the recording will be saved.";
			property = "KH_ModuleRecordMissionIdentifier";
			defaultValue = "''";
		};
		class KH_ModuleRecordMissionUnits: Checkbox
		{
			displayName = "Units";
			tooltip = "True records units.";
			property = "KH_ModuleRecordMissionUnits";
			defaultValue = "true";
		};
		class KH_ModuleRecordMissionGroups: Checkbox
		{
			displayName = "Groups";
			tooltip = "True records groups.";
			property = "KH_ModuleRecordMissionGroups";
			defaultValue = "true";
		};
		class KH_ModuleRecordMissionObjects: Checkbox
		{
			displayName = "Objects";
			tooltip = "True records objects.";
			property = "KH_ModuleRecordMissionObjects";
			defaultValue = "true";
		};
		class KH_ModuleRecordMissionScenario: Checkbox
		{
			displayName = "Scenario";
			tooltip = "True records the scenario, such as environmental factors and side relations.";
			property = "KH_ModuleRecordMissionScenario";
			defaultValue = "true";
		};
		class KH_ModuleRecordMissionCaptureFinalData: Checkbox
		{
			displayName = "Capture Final Data";
			tooltip = "True captures the final version of all captured data in a more verbose format at the end of the mission.";
			property = "KH_ModuleRecordMissionCaptureFinalData";
			defaultValue = "true";
		};
		class ModuleDescription: ModuleDescription {};
	};
	class ModuleDescription: ModuleDescription
	{
		description[] = {"Records everything that happened during the mission and saves it to a KH Data file; useful for mission analysis or replay. Activates only once, with a trigger, on the server."};
		sync[] = {};
		position = 0;
		direction = 0;
		optional = 1;
		duplicate = 1;
	};
};