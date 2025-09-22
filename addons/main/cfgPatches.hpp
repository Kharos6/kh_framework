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
			"KH_ModuleCameraSequence",
			"KH_ModuleDynamicDisguiseEquipment",
			"KH_ModuleEditTerrainObjects",
			"KH_ModuleEndMission",
			"KH_ModuleEntitySpawner",
			"KH_ZeusModuleArma",
			"KH_ZeusModuleBreakDisguise",
			"KH_ZeusModuleEndMission",
			"KH_ZeusModuleRagdollUnit",
			"KH_ZeusModuleSuspendMission"
		};
		weapons[] = {};
		requiredVersion = 1;
		requiredAddons[] = 
		{
			"cba_main",
			"Intercept_Core"
		};
		skipWhenMissingDependencies = 1;
	};
};