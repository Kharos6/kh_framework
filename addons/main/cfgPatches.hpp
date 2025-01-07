class CfgPatches
{
	class KH_Framework
	{
		name = "KH Framework";
		author = "Kharos";
		units[] = 
		{
			"KH_HelperSquare",
			"KH_HelperArrow",
			"KH_ModuleAddInteractionEntry",
			"KH_ModuleAssignParachute",
			"KH_ModuleBreakDisguise",
			"KH_ModuleCameraSequence",
			"KH_ModuleDisplaySubtitle",
			"KH_ModuleDynamicDisguiseEquipment",
			"KH_ModuleEditTerrainObjects",
			"KH_ModuleEndMission",
			"KH_ModuleEntitySpawner",
			"KH_ModulePlayMusic",
			"KH_ModulePlaySound2D",
			"KH_ModulePlaySound3D",
			"KH_ModuleSetAnimation",
			"KH_ModuleSuspendMission",
			"KH_ModuleTeleport",
			"KH_ModuleVehicleSequence",
			"KH_ZeusModuleArma",
			"KH_ZeusModuleBreakDisguise",
			"KH_ZeusModuleClearStagedExecutions",
			"KH_ZeusModuleEndMission",
			"KH_ZeusModuleRagdollUnit",
			"KH_ZeusModuleStageExecution",
			"KH_ZeusModuleSuspendMission"
		};
		weapons[] = {};
		requiredVersion = 1;
		requiredAddons[] = 
		{
			"A3_Modules_F",
			"cba_main"
		};
		skipWhenMissingDependencies = 1;
	};
};