class KH_ModuleEndMission: Module_F
{
	author = "Kharos";
	scope = 2;
	scopeCurator = 1;
	canSetArea = 0;
	canSetAreaHeight = 0;
	canSetAreaShape = 0;
	displayName = "End Mission";
	category = "KH_Modules";
	function = "KH_fnc_moduleEndMission";
	functionPriority = 1;
	isGlobal = 0;
	isTriggerActivated = 1;
	isDisposable = 1;
	is3DEN = 0;
	curatorInfoType = "";
	curatorInfoTypeEmpty = "";
	curatorCanAttach = 0;
	icon = "\a3\modules_f_curator\data\iconendmission_ca.paa";
	class Attributes: AttributesBase
	{
		class KH_ModuleEndMissionName: Edit
		{
			displayName = "Name";
			tooltip = "Class name from CfgDebriefing.";
			property = "KH_ModuleEndMissionName";
			defaultValue = "'KH_MissionConcluded'";
		};
		class KH_ModuleEndMissionVictory: Checkbox
		{
			displayName = "Victory";
			tooltip = "True considers the end a victory.";
			property = "KH_ModuleEndMissionVictory";
			defaultValue = "true";
		};
		class KH_ModuleEndMissionFadeType: Edit
		{
			displayName = "Fade Type";
			tooltip = "Type of fade to play after the specified delay. -2 plays the signature closing shot and music, -1 instantly ends the mission without the signature closing shot and music, and any value of 0 and above is a fade to black for that amount of seconds.";
			property = "KH_ModuleEndMissionFadeType";
			defaultValue = "'-2'";
		};
		class KH_ModuleEndMissionDelay: Edit
		{
			displayName = "Delay";
			tooltip = "Time to wait before initiating the end of the mission, in seconds. Will clamp to a minimum of 1 second if below 1.";
			property = "KH_ModuleEndMissionDelay";
			defaultValue = "'1'";
		};
		class KH_ModuleEndMissionNextMission: Edit
		{
			displayName = "Next Mission";
			tooltip = "The string of the server password followed by the string of the name of the next mission. Leave empty if no subsequent mission is desired. If this attribute is used, all other attributes are ignored, and the next mission will start immediately after the delay.";
			property = "KH_ModuleEndMissionNextMission";
			defaultValue = "''";
		};
		class ModuleDescription: ModuleDescription {};
	};
	class ModuleDescription: ModuleDescription
	{
		description[] = {"Ends the mission, saving any persistency values if the persistency system is enabled and optionally starting the next mission. Activates only once, with a trigger, on the server."};
		sync[] = {};
		position = 0;
		direction = 0;
		optional = 1;
		duplicate = 1;
	};
};