class KH_ModuleEndMission: Module_F
{
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
	icon = "\a3\modules_f_curator\data\iconendmission_ca.paa";
	curatorInfoType = "";
	curatorInfoTypeEmpty = "";
	curatorCanAttach = 0;
	class Attributes: AttributesBase
	{
		class KH_ModuleEndMissionEndName: Edit
		{
			displayName = "End Name";
			tooltip = "End name from <configFile >> 'CfgDebriefing'>.";
			property = "KH_ModuleEndMissionEndName";
			defaultValue = "''";
		};
		class KH_ModuleEndMissionVictory: Checkbox
		{
			displayName = "Victory";
			tooltip = "<true> considers the end a victory.";
			property = "KH_ModuleEndMissionVictory";
			defaultValue = "true";
		};
		class KH_ModuleEndMissionFadeType: Edit
		{
			displayName = "Fade Type";
			tooltip = "Type of fade to play after the specified <Delay> time. <-2> plays the signature closing shot and music, <-1> instantly ends the mission without the signature closing shot and music, and any value of <0> and above is a fade to black for that amount of seconds.";
			property = "KH_ModuleEndMissionFadeType";
			defaultValue = "''";
		};
		class KH_ModuleEndMissionDelay: Edit
		{
			displayName = "Delay";
			tooltip = "Time to wait before initiating the end of the mission, in seconds. Will clamp to a minimum of 1 second if below <1>.";
			property = "KH_ModuleEndMissionDelay";
			defaultValue = "''";
		};
		class KH_ModuleEndMissionNextMission: Edit
		{
			displayName = "Next Mission";
			tooltip = "An array containing the string of the server password, if any, and the string of the name of the next mission, in format <[_password (STRING), _nextMission (STRING)]>. Can be an empty array in case of no subsequent mission. If this value is empty, or an empty array, it will be ignored. If this value is used, all other values are ignored, and the next mission will start immediately after <Delay> seconds.";
			property = "KH_ModuleEndMissionNextMission";
			defaultValue = "'[]'";
		};
		class ModuleDescription: ModuleDescription {};
	};
	class ModuleDescription: ModuleDescription
	{
		description[] = {"Ends the mission, saving the mission state if that was enabled, and optionally starting the next mission. Can be exclusively activated through a trigger or triggers. Can activate only once. Activated on the server."};
	};
};