class KH_ModuleSuspendMission: Module_F
{
	scope = 2;
	scopeCurator = 1;
	canSetArea = 0;
	canSetAreaHeight = 0;
	canSetAreaShape = 0;
	displayName = "Suspend Mission";
	category = "KH_Modules";
	function = "KH_fnc_moduleSuspendMission";
	functionPriority = 1;
	isGlobal = 0;
	isTriggerActivated = 1;
	isDisposable = 0;
	is3DEN = 0;
	icon = "\a3\modules_f_curator\data\iconendmission_ca.paa";
	curatorInfoType = "";
	curatorInfoTypeEmpty = "";
	curatorCanAttach = 0;
	class Attributes: AttributesBase
	{
		class KH_ModuleSuspendMissionSuspend: Checkbox
		{
			displayName = "Suspend";
			tooltip = "<true> suspends the mission. <false> resumes the mission.";
			property = "KH_ModuleSuspendMissionSuspend";
			defaultValue = "false";
		};
		class ModuleDescription: ModuleDescription {};
	};
	class ModuleDescription: ModuleDescription
	{
		description[] = {"Suspends or resumes the mission, depending on the state of Suspend, disabling simulation on all mission entities, and notifying the players about the suspension. Must be activated through a trigger or triggers. Can activate multiple times. Activated on the server."};
	};
};