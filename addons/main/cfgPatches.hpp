class CfgPatches
{
	class KH_Framework
	{
		name = "KH Framework";
		author = "Kharos";
		units[] = {
			"KH_ZeusModuleArma",
			"KH_ZeusModuleBreakDisguise",
			"KH_ZeusModuleDisembarkUnits",
			"KH_ZeusModuleEndMission",
			"KH_ZeusModuleHeal",
			"KH_ZeusModuleHealAllPlayers",
			"KH_ZeusModuleKill",
			"KH_ZeusModuleRagdollUnit",
			"KH_ZeusModuleToggleMissionSuspension"
		};
		weapons[] = {};
		requiredVersion = 2.20;
		requiredAddons[] = {"cba_main", "Intercept_Core"};
		skipWhenMissingDependencies = 1;
	};
};