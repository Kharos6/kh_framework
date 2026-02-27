[
	"KH_var_uiEnabledElementColor",
	"COLOR",
	[
		"UI Enabled Element Color", 
		"Personalized color choice for enabled UI elements."
	], 
	"KH User Interface", 
	[1, 0.6, 0.1, 0.75],
	0,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_uiDisabledElementColor",
	"COLOR",
	[
		"UI Disabled Element Color", 
		"Personalized color choice for disabled UI elements."
	], 
	"KH User Interface", 
	[1, 0.6, 0.1, 0.45],
	0,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_uiActiveElementColor",
	"COLOR",
	[
		"UI Active Element Color", 
		"Personalized color choice for active UI elements."
	], 
	"KH User Interface", 
	[1, 0.6, 0.1, 1],
	0,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_uiForegroundColor",
	"COLOR",
	[
		"UI Foreground Color", 
		"Personalized UI foreground color choice."
	],
	"KH User Interface", 
	[0, 0, 0, 0.9],
	0,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_uiBackgroundColor",
	"COLOR",
	[
		"UI Background Color", 
		"Personalized UI background color choice."
	],
	"KH User Interface", 
	[0, 0, 0, 0.45],
	0,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_allowRemoteInventories",
	"CHECKBOX",   
	[
		"Allow Remote Inventories", 
		"True allows players to open the inventory of other units."
	], 
	"KH Miscellaneous", 
	false,
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_anchorPlayersToGeometry",
	"CHECKBOX",   
	[
		"Anchor Players To Geometry", 
		"True allows players to walk on geometry that would otherwise cause falling. May or may not be volatile, depending on the surface in question."
	], 
	"KH Miscellaneous", 
	false,
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_anchorPlayersToMovingObjects",
	"LIST",   
	[
		"Anchor Players To Moving Objects", 
		"Defines whether players can walk on moving objects such as vehicles or physics objects. May or may not be volatile, depending on the surface in question. Position based uses reliable position snapping, but results in rubber banding for other clients in multiplayer. Velocity based uses velocity blending that looks better in multiplayer, but can cause jittering in first person during high speeds and may result in a space program. Variable uses velocity when the speed is below 10 metres per second and position when the speed is above 10 metres per second, granting the best of both worlds."
	], 
	"KH Miscellaneous", 
	[
        [0, 1, 2, 3],
        ["NONE", "VELOCITY BASED", "POSITION BASED", "VARIABLE"],
        0
    ],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_fuelSiphoning",
	"CHECKBOX",   
	[
		"Enable", 
		"True allows players to siphon fuel from one vehicle to another."
	], 
	[
		"KH Engineering",
		"Fuel Siphoning"
	],
	false,
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_fuelSiphoningRequireToolkit",
	"CHECKBOX",   
	[
		"Require Toolkit", 
		"True makes it so that the toolkit is required in order to use the fuel siphoning system."
	], 
	[
		"KH Engineering",
		"Fuel Siphoning"
	],
	true,
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_fuelSiphoningEngineerOnly",
	"CHECKBOX",   
	[
		"Engineer Only", 
		"True makes it so that only engineers can siphon fuel."
	], 
	[
		"KH Engineering",
		"Fuel Siphoning"
	],
	false,
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_fuelSiphoningSpeed",
	"SLIDER",   
	[
		"Speed", 
		"The amount of fuel siphoned per second. 1.000 is 100% fuel, 0.000 is 0% fuel."
	], 
	[
        "KH Engineering",
        "Fuel Siphoning"
    ], 
	[0.000, 1.000, 0.010, 3],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_medical",
	"CHECKBOX",   
	[
		"Enable", 
		"True enables the KH Medical system. Requires restart."
	], 
	"KH Medical", 
	false,
	1,
	{},
	true
] call CBA_fnc_addSetting;

[
	"KH_var_incapacitationAvailability",
	"LIST",   
	[
		"Incapacitation Availability", 
		"Defines whether the incapacitation system is enabled, and for whom."
	], 
	[
        "KH Medical",
        "Incapacitation"
    ], 
	[
        [0, 1, 2],
        ["NONE", "PLAYERS", "PLAYERS & AI"],
        1
    ],
	1,
	{
		[
			[],
			{
				switch KH_var_incapacitationAvailability do {
					case 0: {
						KH_var_allowIncapacitation = false;
						KH_var_allowAiIncapacitation = false;
					};

					case 1: {
						KH_var_allowIncapacitation = true;
						KH_var_allowAiIncapacitation = false;
					};

					case 2: {
						KH_var_allowIncapacitation = true;
						KH_var_allowAiIncapacitation = true;
					};

					default {
						KH_var_allowIncapacitation = true;
						KH_var_allowAiIncapacitation = false;
					};
				};

				publicVariable "KH_var_allowIncapacitation";
				publicVariable "KH_var_allowAiIncapacitation";
			},
			"SERVER",
			"1",
			false
		] call KH_fnc_execute;
	},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_incapacitationThreshold",
	"SLIDER",   
	[
		"Incapacitation Threshold", 
		"The amount of damage to total health beyond which a unit will be incapacitated. 1.00 is 100% damage, 0.00 is 0% damage."
	], 
	[
        "KH Medical",
        "Incapacitation"
    ], 
	[0.00, 1.00, 0.90, 2],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_incapacitationDamageSpillover",
	"CHECKBOX",   
	[
		"Incapacitation Damage Spillover", 
		"True makes it so that damage that would spill over the incapacitation threshold is recalculated with the Incapacitated Damage Multiplier, while false clamps the spilled over damage to the Incapacitation Threshold to prevent instant deaths from extreme damage."
	], 
	[
        "KH Medical",
        "Incapacitation"
    ], 
	true,
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_incapacitationDeathTime",
	"TIME",   
	[
		"Incapacitation Death Time", 
		"The time it takes for an incapacitated unit to die."
	], 
	[
        "KH Medical",
        "Incapacitation"
    ], 
	[0, 1800, 300],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_allowGivingUp",
	"CHECKBOX",   
	[
		"Allow Giving Up", 
		"True allows incapacitated players to give up and die."
	], 
	[
        "KH Medical",
        "Incapacitation"
    ], 
	true,
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_incapacitatedCaptives",
	"CHECKBOX",   
	[
		"Incapacitated Captives", 
		"True sets incapacitated units as captives to prevent executions by enemies."
	], 
	[
        "KH Medical",
        "Incapacitation"
    ], 
	true,
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_reviveRequireStabilization",
	"CHECKBOX",   
	[
		"Revive Require Stabilization", 
		"True requires a unit to be stabilized before they can be revived."
	], 
	[
        "KH Medical",
        "Incapacitation"
    ], 
	false,
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_reviveDuration",
	"TIME",   
	[
		"Revive Duration", 
		"Time it takes to revive another person."
	], 
	[
        "KH Medical",
        "Incapacitation"
    ], 
	[0, 300, 15],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_reviveMedicOnly",
	"CHECKBOX",   
	[
		"Revive Medic Only", 
		"True only allows medics to revive incapacitated units."
	], 
	[
        "KH Medical",
        "Incapacitation"
    ], 
	false,
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_reviveMedicDuration",
	"TIME",   
	[
		"Revive Medic Duration", 
		"Time it takes for medics to revive another person."
	], 
	[
        "KH Medical",
        "Incapacitation"
    ], 
	[0, 300, 10],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_reviveRequireMedikit",
	"CHECKBOX",   
	[
		"Revive Require Medikit", 
		"True makes the medikit required to revive an incapacitated unit."
	], 
	[
        "KH Medical",
        "Incapacitation"
    ], 
	false,
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_reviveRequiredFirstAidKits",
	"SLIDER",   
	[
		"Revive Required First Aid Kits", 
		"Amount of first aid kits required for reviving a person when no medikit is available and Revive Require Medikit is false."
	], 
	[
        "KH Medical",
        "Incapacitation"
    ], 
	[0, 15, 2, 0],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_selfRevive",
	"CHECKBOX",   
	[
		"Self Revive", 
		"True allows a person to revive themselves."
	], 
	[
        "KH Medical",
        "Incapacitation"
    ], 
	true,
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_selfReviveDuration",
	"TIME",   
	[
		"Self Revive Duration", 
		"Time it takes for a person to revive themselves."
	], 
	[
        "KH Medical",
        "Incapacitation"
    ], 
	[0, 300, 30],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_selfReviveMedicOnly",
	"CHECKBOX",   
	[
		"Self Revive Medic Only", 
		"True only allows medics to revive themselves."
	], 
	[
        "KH Medical",
        "Incapacitation"
    ], 
	false,
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_selfReviveMedicDuration",
	"TIME",   
	[
		"Self Revive Medic Duration", 
		"Time it takes for medics to revive themselves."
	], 
	[
        "KH Medical",
        "Incapacitation"
    ], 
	[0, 300, 20],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_selfReviveRequireMedikit",
	"CHECKBOX",   
	[
		"Self Revive Require Medikit", 
		"True makes the medikit required for a person to revive themselves."
	], 
	[
        "KH Medical",
        "Incapacitation"
    ], 
	false,
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_selfReviveRequiredFirstAidKits",
	"SLIDER",   
	[
		"Self Revive Required First Aid Kits", 
		"Amount of first aid kits required for a person to revive themselves when no medikit is available and Self Revive Require Medikit is false."
	], 
	[
        "KH Medical",
        "Incapacitation"
    ], 
	[0, 15, 3, 0],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_selfReviveRequireWithstanding",
	"CHECKBOX",   
	[
		"Self Revive Require Withstanding", 
		"True only allows players to revive themselves if they are also withstanding."
	], 
	[
        "KH Medical",
        "Incapacitation"
    ], 
	true,
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_batchHeal",
	"CHECKBOX",
	[
		"Batch Heal", 
		"True makes it so that as many first aid kits as are necessary are used in order to restore full health. If false, healing needs to be conducted one action at a time until health is fully restored."
	], 
	[
        "KH Medical",
        "Healing"
    ], 
	true,
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_firstAidKitHeal",
	"SLIDER",   
	[
		"First Aid Kit Heal",
		"Amount of health added per first aid kit used on another person. 1.00 is 100% health, 0.00 is 0% health."
	], 
	[
        "KH Medical",
        "Healing"
    ], 
	[0.00, 1.00, 0.50, 2],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_firstAidKitSelfHeal",
	"SLIDER",   
	[
		"First Aid Kit Self Heal",
		"Amount of health added per first aid kit used by a person on themselves. 1.00 is 100% health, 0.00 is 0% health."
	], 
	[
        "KH Medical",
        "Healing"
    ], 
	[0.00, 1.00, 0.50, 2],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_firstAidKitHealMedic",
	"SLIDER",   
	[
		"First Aid Kit Heal Medic",
		"Amount of health added per first aid kit used by a medic on another person. 1.00 is 100% health, 0.00 is 0% health."
	], 
	[
        "KH Medical",
        "Healing"
    ], 
	[0.00, 1.00, 1.00, 2],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_firstAidKitSelfHealMedic",
	"SLIDER",   
	[
		"First Aid Kit Self Heal Medic",
		"Amount of health added per first aid kit used by a medic on themselves. 1.00 is 100% health, 0.00 is 0% health."
	], 
	[
        "KH Medical",
        "Healing"
    ], 
	[0.00, 1.00, 1.00, 2],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_firstAidKitHealWithstanding",
	"SLIDER",   
	[
		"First Aid Kit Heal Withstanding",
		"Amount of health added per first aid kit used on another person that is withstanding. 1.00 is 100% health, 0.00 is 0% health."
	], 
	[
        "KH Medical",
        "Healing"
    ], 
	[0.00, 1.00, 0.25, 2],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_firstAidKitSelfHealWithstanding",
	"SLIDER",   
	[
		"First Aid Kit Self Heal Withstanding",
		"Amount of health added per first aid kit used by a person on themselves while they are withstanding. 1.00 is 100% health, 0.00 is 0% health."
	], 
	[
        "KH Medical",
        "Healing"
    ], 
	[0.00, 1.00, 0.25, 2],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_firstAidKitHealWithstandingMedic",
	"SLIDER",   
	[
		"First Aid Kit Heal Withstanding Medic",
		"Amount of health added per first aid kit used by a medic on another person that is withstanding. 1.00 is 100% health, 0.00 is 0% health."
	], 
	[
        "KH Medical",
        "Healing"
    ], 
	[0.00, 1.00, 0.50, 2],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_firstAidKitSelfHealWithstandingMedic",
	"SLIDER",   
	[
		"First Aid Kit Self Heal Withstanding Medic",
		"Amount of health added per first aid kit used by a medic on themselves while they are withstanding. 1.00 is 100% health, 0.00 is 0% health."
	], 
	[
        "KH Medical",
        "Healing"
    ], 
	[0.00, 1.00, 0.50, 2],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_reviveHeal",
	"SLIDER",   
	[
		"Revive Heal",
		"Amount of health that is recovered after reviving. 1.00 is 100% health, 0.00 is 0% health. If the amount of health results in the remaining damage being above the incapacitation threshold, the next hit on the revived unit will immediately incapacitate them."
	], 
	[
        "KH Medical",
        "Healing"
    ], 
	[0.00, 1.00, 0.50, 2],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_reviveHealMedic",
	"SLIDER",   
	[
		"Revive Heal Medic",
		"Amount of health that is recovered by medics after reviving. 1.00 is 100% health, 0.00 is 0% health. If the amount of health results in the remaining damage being above the incapacitation threshold, the next hit on the revived unit will immediately incapacitate them."
	], 
	[
        "KH Medical",
        "Healing"
    ], 
	[0.00, 1.00, 1.00, 2],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_selfReviveHeal",
	"SLIDER",   
	[
		"Self Revive Heal",
		"Amount of health that is recovered after a unit revives itself. 1.00 is 100% health, 0.00 is 0% health. If the amount of health results in the remaining damage being above the incapacitation threshold, the next hit on the revived unit will immediately incapacitate them."
	], 
	[
        "KH Medical",
        "Healing"
    ], 
	[0.00, 1.00, 0.50, 2],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_selfReviveHealMedic",
	"SLIDER",   
	[
		"Self Revive Heal Medic",
		"Amount of health that is recovered by medics after they revive themselves. 1.00 is 100% health, 0.00 is 0% health. If the amount of health results in the remaining damage being above the incapacitation threshold, the next hit on the revived unit will immediately incapacitate them."
	], 
	[
        "KH Medical",
        "Healing"
    ], 
	[0.00, 1.00, 1.00, 2],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_stabilizationAllowed",
	"CHECKBOX",   
	[
		"Enable", 
		"True allows players to stabilize incapacitated units. Stabilization stops the Incapacitation Death Time countdown."
	], 
	[
        "KH Medical",
        "Stabilization"
    ], 
	true,
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_stabilizationDuration",
	"TIME",   
	[
		"Stabilization Duration", 
		"Time it takes to stabilize a person."
	], 
	[
        "KH Medical",
        "Stabilization"
    ], 
	[0, 300, 15],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_stabilizationMedicOnly",
	"CHECKBOX",   
	[
		"Stabilization Medic Only", 
		"True only allows medics to stabilize incapacitated units."
	], 
	[
        "KH Medical",
        "Stabilization"
    ], 
	false,
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_stabilizationDurationMedic",
	"TIME",   
	[
		"Stabilization Duration Medic", 
		"Time it takes for medics to stabilize a person."
	], 
	[
        "KH Medical",
        "Stabilization"
    ], 
	[0, 300, 10],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_stabilizationRequireMedikit",
	"CHECKBOX",   
	[
		"Stabilization Require Medikit", 
		"True makes the medikit required to stabilize an incapacitated unit."
	], 
	[
        "KH Medical",
        "Stabilization"
    ], 
	false,
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_stabilizationRequiredFirstAidKits",
	"SLIDER",   
	[
		"Stabilization Required First Aid Kits", 
		"Amount of first aid kits required for stabilizing a person when no medikit is available and Stabilization Require Medikit is false."
	], 
	[
        "KH Medical",
        "Stabilization"
    ], 
	[0, 15, 1, 0],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_selfStabilization",
	"CHECKBOX",   
	[
		"Self Stabilization", 
		"True allows a person to stabilize themselves."
	], 
	[
        "KH Medical",
        "Stabilization"
    ], 
	true,
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_selfStabilizationDuration",
	"TIME",   
	[
		"Self Stabilization Duration", 
		"Time it takes for a person to stabilize themselves."
	], 
	[
        "KH Medical",
        "Stabilization"
    ], 
	[0, 300, 20],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_selfStabilizationMedicOnly",
	"CHECKBOX",   
	[
		"Self Stabilization Medic Only", 
		"True only allows medics to stabilize themselves."
	], 
	[
        "KH Medical",
        "Stabilization"
    ], 
	false,
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_selfStabilizationDurationMedic",
	"TIME",   
	[
		"Self Stabilization Duration Medic", 
		"Time it takes for medics to stabilize themselves."
	], 
	[
        "KH Medical",
        "Stabilization"
    ], 
	[0, 300, 15],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_selfStabilizationRequireMedikit",
	"CHECKBOX",   
	[
		"Self Stabilization Require Medikit", 
		"True makes the medikit required for a person to stabilize themselves."
	], 
	[
        "KH Medical",
        "Stabilization"
    ], 
	false,
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_selfStabilizationRequiredFirstAidKits",
	"SLIDER",   
	[
		"Self Stabilization Required First Aid Kits", 
		"Amount of first aid kits required for a person to stabilize themselves when no medikit is available and Self Stabilization Require Medikit is false."
	], 
	[
        "KH Medical",
        "Stabilization"
    ], 
	[0, 15, 2, 0],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_withstandingAllowed",
	"CHECKBOX",   
	[
		"Enable", 
		"True allows players to withstand if incapacitated. Withstanding allows players to continue fighting by getting back up, but makes them susceptible to mortal wounds. Healing while withstanding will only recover health, but will not prevent mortal wounds until properly revived."
	], 
	[
        "KH Medical",
        "Withstanding"
    ], 
	true,
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_withstandingRequireStabilization",
	"CHECKBOX",   
	[
		"Require Stabilization", 
		"True only allows players to withstand if they are stabilized."
	], 
	[
        "KH Medical",
        "Withstanding"
    ], 
	false,
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_withstandDelay",
	"TIME",   
	[
		"Delay", 
		"Time required for a player to be incapacitated before they are able to withstand."
	], 
	[
        "KH Medical",
        "Withstanding"
    ], 
	[0, 1800, 10],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_withstandActionDuration",
	"TIME",   
	[
		"Action Duration", 
		"The time it takes to start and cancel withstanding."
	], 
	[
        "KH Medical",
        "Withstanding"
    ], 
	[0, 60, 3],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_withstandDuration",
	"TIME",   
	[
		"Duration", 
		"Time it takes for a withstanding player to collapse into the incapacitation state again, with no more withstanding ability until they are revived. Set to 0 for infinite withstanding."
	], 
	[
        "KH Medical",
        "Withstanding"
    ], 
	[0, 1800, 180],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_withstandingAllowCancel",
	"CHECKBOX",   
	[
		"Allow Cancel", 
		"True allows players who are withstanding to cancel withstanding and become incapacitated again. This will prevent further withstanding until revived."
	], 
	[
        "KH Medical",
        "Withstanding"
    ], 
	true,
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_withstandingAllowRevivingOthers",
	"CHECKBOX",   
	[
		"Allow Reviving Others", 
		"True allows players who are withstanding to revive other players."
	], 
	[
        "KH Medical",
        "Withstanding"
    ], 
	true,
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_absoluteTotalDamageMultiplier",
	"SLIDER",   
	[
		"Total Damage Multiplier", 
		"All damage calculations for total unit health are multiplied by this value."
	], 
	[
        "KH Medical",
        "Damage Multipliers"
    ], 
	[0.000, 10.000, 1.000, 3],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_absoluteDamageMultiplierHead",
	"SLIDER",   
	[
		"Head Damage Multiplier", 
		"All damage calculations for the head are multiplied by this value."
	], 
	[
        "KH Medical",
        "Damage Multipliers"
    ], 
	[0.000, 10.000, 1.000, 3],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_absoluteDamageMultiplierNeck",
	"SLIDER",   
	[
		"Neck Damage Multiplier", 
		"All damage calculations for the neck are multiplied by this value."
	], 
	[
        "KH Medical",
        "Damage Multipliers"
    ], 
	[0.000, 10.000, 1.000, 3],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_absoluteDamageMultiplierArm",
	"SLIDER",   
	[
		"Arm Damage Multiplier", 
		"All damage calculations for arms are multiplied by this value."
	], 
	[
        "KH Medical",
        "Damage Multipliers"
    ], 
	[0.000, 10.000, 1.000, 3],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_absoluteDamageMultiplierLeg",
	"SLIDER",   
	[
		"Leg Damage Multiplier", 
		"All damage calculations for legs are multiplied by this value."
	], 
	[
        "KH Medical",
        "Damage Multipliers"
    ], 
	[0.000, 10.000, 1.000, 3],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_absoluteDamageMultiplierTorso",
	"SLIDER",   
	[
		"Torso Damage Multiplier", 
		"All damage calculations for the torso are multiplied by this value."
	], 
	[
        "KH Medical",
        "Damage Multipliers"
    ], 
	[0.000, 10.000, 1.000, 3],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_absoluteDamageMultiplierPelvis",
	"SLIDER",   
	[
		"Pelvis Damage Multiplier", 
		"All damage calculations for the pelvis are multiplied by this value."
	], 
	[
        "KH Medical",
        "Damage Multipliers"
    ], 
	[0.000, 10.000, 1.000, 3],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_absoluteImpactDamageMultiplier",
	"SLIDER",   
	[
		"Impact Damage Multiplier", 
		"All damage calculations for impact damage, such as from falling, are multiplied by this value."
	], 
	[
        "KH Medical",
        "Damage Multipliers"
    ], 
	[0.000, 10.000, 1.000, 3],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_absoluteFriendlyDamageMultiplier",
	"SLIDER",   
	[
		"Friendly Damage Multiplier", 
		"All friendly damage calculations are multiplied by this value."
	], 
	[
        "KH Medical",
        "Damage Multipliers"
    ], 
	[0.000, 10.000, 1.000, 3],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_absoluteMeleeAttackDamageMultiplier",
	"SLIDER",   
	[
		"Melee Attack Damage Multiplier", 
		"All melee attack damage calculations are multiplied by this value."
	], 
	[
        "KH Medical",
        "Damage Multipliers"
    ], 
	[0.000, 10.000, 1.000, 3],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_absoluteMeleeKickDamageMultiplier",
	"SLIDER",   
	[
		"Melee Kick Damage Multiplier", 
		"All melee kick damage calculations are multiplied by this value."
	], 
	[
        "KH Medical",
        "Damage Multipliers"
    ], 
	[0.000, 10.000, 1.000, 3],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_absoluteMeleeTackleInstigatorDamageMultiplier",
	"SLIDER",   
	[
		"Melee Tackle Instigator Damage Multiplier", 
		"All melee damage calculations on tacklers are multiplied by this value."
	], 
	[
        "KH Medical",
        "Damage Multipliers"
    ], 
	[0.000, 10.000, 1.000, 3],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_absoluteMeleeDamageMultiplier",
	"SLIDER",   
	[
		"Melee Damage Multiplier", 
		"All melee damage calculations are multiplied by this value."
	], 
	[
        "KH Medical",
        "Damage Multipliers"
    ], 
	[0.000, 10.000, 1.000, 3],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_absoluteMeleeFriendlyDamageMultiplier",
	"SLIDER",   
	[
		"Melee Friendly Damage Multiplier", 
		"All friendly melee damage calculations are multiplied by this value."
	], 
	[
        "KH Medical",
        "Damage Multipliers"
    ], 
	[0.000, 10.000, 1.000, 3],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_absoluteDamageMultiplier",
	"SLIDER",   
	[
		"Damage Multiplier", 
		"All damage calculations for all hit points and total unit health are multiplied by this value."
	], 
	[
        "KH Medical",
        "Damage Multipliers"
    ], 
	[0.000, 10.000, 1.000, 3],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_absoluteAiDamageMultiplier",
	"SLIDER",   
	[
		"AI Damage Multiplier", 
		"All damage calculations for all hit points and total unit health on AI are multiplied by this value."
	], 
	[
        "KH Medical",
        "Damage Multipliers"
    ], 
	[0.000, 10.000, 1.000, 3],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_absolutePlayerDamageMultiplier",
	"SLIDER",   
	[
		"Player Damage Multiplier", 
		"All damage calculations for all hit points and total unit health on players are multiplied by this value."
	], 
	[
        "KH Medical",
        "Damage Multipliers"
    ], 
	[0.000, 10.000, 1.000, 3],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_absoluteIncapacitatedDamageMultiplier",
	"SLIDER",   
	[
		"Incapacitated Damage Multiplier", 
		"All damage calculations for all hit points and total unit health while a unit is incapacitated are multiplied by this value."
	], 
	[
        "KH Medical",
        "Damage Multipliers"
    ], 
	[0.000, 10.000, 0.500, 3],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_absoluteWithstandingDamageMultiplier",
	"SLIDER",   
	[
		"Withstanding Damage Multiplier", 
		"All damage calculations for all hit points and total unit health while a player is withstanding are multiplied by this value."
	], 
	[
        "KH Medical",
        "Damage Multipliers"
    ], 
	[0.000, 10.000, 2.000, 3],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_classDamageMultipliersRaw",
	"EDITBOX",   
	[
		"Class Damage Multipliers", 
		"Hashmap style arrays where the key element is a string of a class name, and the value is its damage multiplier. All units of a specified class will have all damage calculations for all hit points and total unit health multiplied by the value. Requires restart."
	], 
	[
        "KH Medical",
        "Damage Multipliers"
    ], 
	"",
	1,
	{},
	true
] call CBA_fnc_addSetting;

[
	"KH_var_childClassDamageMultipliersRaw",
	"EDITBOX",   
	[
		"Child Class Damage Multipliers", 
		"Hashmap style arrays where the key element is a string of a class name, and the value is its damage multiplier. All units whose classes are children of a specified class will have all damage calculations for all hit points and total unit health multiplied by the value. Requires restart."
	], 
	[
        "KH Medical",
        "Damage Multipliers"
    ], 
	"",
	1,
	{},
	true
] call CBA_fnc_addSetting;

[
	"KH_var_healthDisplay",
	"LIST",   
	[
		"Health Display", 
		"The type of health representation displayed to a player, representing their own health."
	], 
	[
        "KH Medical",
        "Interface"
    ], 
	[
        ["NONE", "BAR", "PERCENTAGE"],
        ["NONE", "BAR", "PERCENTAGE"],
        1
    ],
	0,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_healthDisplayHideFullHealth",
	"CHECKBOX",   
	[
		"Health Display Hide Full Health", 
		"True hides the health display if health is full."
	], 
	[
        "KH Medical",
        "Interface"
    ], 
	false,
	0,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_healthDisplayBarPositionX",
	"SLIDER",   
	[
		"Health Display Bar Position X", 
		"X axis position of the bar health display. 100.00 is the left edge of the screen, 0.00 is the right edge of the screen."
	], 
	[
        "KH Medical",
        "Interface"
    ], 
	[0.00, 100.00, 2.50, 2],
	0,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_healthDisplayBarPositionY",
	"SLIDER",   
	[
		"Health Display Bar Position Y", 
		"Y axis position of the bar health display. 100.00 is the top edge of the screen, 0.00 is the bottom edge of the screen."
	], 
	[
        "KH Medical",
        "Interface"
    ], 
	[0.00, 100.00, 95.00, 2],
	0,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_healthDisplayBarSizeX",
	"SLIDER",   
	[
		"Health Display Bar Size X", 
		"X axis size of the bar health display. The pivot point is on the left edge of the UI element."
	], 
	[
        "KH Medical",
        "Interface"
    ], 
	[0.00, 100.00, 20.00, 2],
	0,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_healthDisplayBarSizeY",
	"SLIDER",   
	[
		"Health Display Bar Size Y", 
		"Y axis size of the bar health display. The pivot point is on the top edge of the UI element."
	], 
	[
        "KH Medical",
        "Interface"
    ], 
	[0.00, 100.00, 1.50, 2],
	0,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_healthDisplayBarAngle",
	"SLIDER",   
	[
		"Health Display Bar Angle", 
		"Angle of the bar health display. The pivot point is on the top left edge of the UI element."
	], 
	[
        "KH Medical",
        "Interface"
    ], 
	[0.00, 360.00, 0.00, 2],
	0,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_healthDisplayPercentagePositionX",
	"SLIDER",   
	[
		"Health Display Percentage Position X", 
		"X axis position of the percentage health display. 100.00 is the left edge of the screen, 0.00 is the right edge of the screen."
	], 
	[
        "KH Medical",
        "Interface"
    ], 
	[0.00, 100.00, 2.50, 2],
	0,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_healthDisplayPercentagePositionY",
	"SLIDER",   
	[
		"Health Display Percentage Position Y", 
		"Y axis position of the percentage health display. 100.00 is the top edge of the screen, 0.00 is the bottom edge of the screen."
	], 
	[
        "KH Medical",
        "Interface"
    ], 
	[0.00, 100.00, 92.50, 2],
	0,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_healthDisplayPercentageSizeX",
	"SLIDER",   
	[
		"Health Display Percentage Size X", 
		"X axis size of the percentage health display. The pivot point is on the left edge of the UI element."
	], 
	[
        "KH Medical",
        "Interface"
    ], 
	[0.00, 100.00, 7.00, 2],
	0,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_healthDisplayPercentageSizeY",
	"SLIDER",   
	[
		"Health Display Percentage Size Y", 
		"Y axis size of the percentage health display. The pivot point is on the top edge of the UI element."
	], 
	[
        "KH Medical",
        "Interface"
    ], 
	[0.00, 100.00, 2.50, 2],
	0,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_healthDisplayPercentageAngle",
	"SLIDER",   
	[
		"Health Display Percentage Angle", 
		"Angle of the percentage health display. The pivot point is on the top left edge of the UI element."
	], 
	[
        "KH Medical",
        "Interface"
    ], 
	[0.00, 360.00, 0.00, 2],
	0,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_healthDisplayPercentageTextSize",
	"SLIDER",   
	[
		"Health Display Percentage Text Size", 
		"Size of the percentage health display text, relative to the standard 0.04 config text size."
	], 
	[
        "KH Medical",
        "Interface"
    ], 
	[0.00, 10.00, 2.00, 2],
	0,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_giveUpProgressDisplay",
	"LIST",   
	[
		"Give Up Progress Display", 
		"The type of progress representation displayed to the person giving up."
	], 
	[
        "KH Medical",
        "Interface"
    ], 
	[
        ["NONE", "BAR", "PERCENTAGE", "TIME"],
        ["NONE", "BAR", "PERCENTAGE", "TIME"],
        1
    ],
	0,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_reviveProgressDisplay",
	"LIST",   
	[
		"Revive Progress Display", 
		"The type of progress representation displayed to the person using a revive action."
	], 
	[
        "KH Medical",
        "Interface"
    ], 
	[
        ["NONE", "BAR", "PERCENTAGE", "TIME"],
        ["NONE", "BAR", "PERCENTAGE", "TIME"],
        1
    ],
	0,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_stabilizationProgressDisplay",
	"LIST",   
	[
		"Stabilization Progress Display", 
		"The type of progress representation displayed to the person stabilizing an incapacitated unit."
	], 
	[
        "KH Medical",
        "Interface"
    ], 
	[
        ["NONE", "BAR", "PERCENTAGE", "TIME"],
        ["NONE", "BAR", "PERCENTAGE", "TIME"],
        1
    ],
	0,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_withstandProgressDisplay",
	"LIST",   
	[
		"Withstand Progress Display", 
		"The type of progress representation displayed to the person using a withstand or cancel withstand action."
	], 
	[
        "KH Medical",
        "Interface"
    ], 
	[
        ["NONE", "BAR", "PERCENTAGE", "TIME"],
        ["NONE", "BAR", "PERCENTAGE", "TIME"],
        1
    ],
	0,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_allowHitPointMaximumDamage",
	"CHECKBOX",   
	[
		"Allow Hit Point Maximum Damage", 
		"True allows maximum damage to hit points, which is otherwise capped to 0.99 if false. This means that maximum damage to critical hit points, such as the head, can kill a unit regardless of their total health."
	], 
	[
        "KH Medical",
        "General"
    ], 
	false,
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_networkingPort",
	"SLIDER",   
	[
		"Port", 
		"Networking port. Requires restart."
	], 
	"KH Networking", 
	[1024, 65535, 21337, 0],
	1,
	{},
	true
] call CBA_fnc_addSetting;

[
	"KH_var_networkingMaximumMessageSize",
	"EDITBOX",   
	[
		"Maximum Message Size", 
		"Maximum payload size in bytes. Requires restart."
	], 
	"KH Networking", 
	"16777216",
	1,
	{},
	true
] call CBA_fnc_addSetting;

[
	"KH_var_networkingReceiveBufferSize",
	"EDITBOX",   
	[
		"Receive Buffer Size", 
		"Socket receive buffer size in bytes. Requires restart."
	], 
	"KH Networking", 
	"262144",
	1,
	{},
	true
] call CBA_fnc_addSetting;

[
	"KH_var_networkingSendBufferSize",
	"EDITBOX",   
	[
		"Send Buffer Size", 
		"Socket send buffer size in bytes. Requires restart."
	], 
	"KH Networking", 
	"262144",
	1,
	{},
	true
] call CBA_fnc_addSetting;

[
	"KH_var_networkingConnectionTimeout",
	"EDITBOX",   
	[
		"Connection Timeout", 
		"Connection timeout in milliseconds. Requires restart."
	], 
	"KH Networking", 
	"5000",
	1,
	{},
	true
] call CBA_fnc_addSetting;

[
	"KH_var_networkingSendTimeout",
	"EDITBOX",   
	[
		"Send Timeout", 
		"Send operation timeout in milliseconds. Requires restart."
	], 
	"KH Networking", 
	"3000",
	1,
	{},
	true
] call CBA_fnc_addSetting;

[
	"KH_var_networkingReceiveTimeout",
	"EDITBOX",   
	[
		"Receive Timeout", 
		"Receive operation timeout in milliseconds. Requires restart."
	], 
	"KH Networking", 
	"3000",
	1,
	{},
	true
] call CBA_fnc_addSetting;

[
	"KH_var_networkingClientStallTimeout",
	"EDITBOX",   
	[
		"Client Stall Timeout", 
		"Time before disconnecting unresponsive client in milliseconds. Requires restart."
	], 
	"KH Networking", 
	"10000",
	1,
	{},
	true
] call CBA_fnc_addSetting;

[
	"KH_var_networkingKeepAliveTime",
	"EDITBOX",   
	[
		"Keep Alive Time", 
		"Time before the first keep-alive probe in milliseconds. Requires restart."
	], 
	"KH Networking", 
	"15000",
	1,
	{},
	true
] call CBA_fnc_addSetting;

[
	"KH_var_networkingKeepAliveInterval",
	"EDITBOX",   
	[
		"Keep Alive Interval", 
		"Interval between keep-alive probes probes in milliseconds. Requires restart."
	], 
	"KH Networking", 
	"1000",
	1,
	{},
	true
] call CBA_fnc_addSetting;

[
	"KH_var_networkingSendBatchSize",
	"EDITBOX",   
	[
		"Send Batch Size", 
		"Maximum messages sent per cycle. Requires restart."
	], 
	"KH Networking", 
	"64",
	1,
	{},
	true
] call CBA_fnc_addSetting;

[
	"KH_var_networkingCompression",
	"CHECKBOX",   
	[
		"Compression", 
		"True enables compression on network packets. Requires restart."
	], 
	"KH Networking", 
	true,
	1,
	{},
	true
] call CBA_fnc_addSetting;

[
	"KH_var_networkingCoalesceMessages",
	"CHECKBOX",   
	[
		"Coalesce Messages", 
		"True bundles multiple simultaneous messages together to reduce overhead. Requires restart."
	], 
	"KH Networking", 
	true,
	1,
	{},
	true
] call CBA_fnc_addSetting;

[
	"KH_var_networkingMaximumCoalesceSize",
	"EDITBOX",   
	[
		"Maximum Coalesce Size", 
		"Maximum total size in bytes for a coalesced packet before it's flushed and a new one is started. Requires restart."
	], 
	"KH Networking", 
	"65536",
	1,
	{},
	true
] call CBA_fnc_addSetting;

[
	"KH_var_networkingMaximumCoalescedMessages",
	"EDITBOX",   
	[
		"Maximum Coalesced Messages", 
		"Maximum number of individual messages that can be bundled into a single coalesced packet. Requires restart."
	], 
	"KH Networking", 
	"128",
	1,
	{},
	true
] call CBA_fnc_addSetting;

[
	"KH_var_networkingCoalesceDelay",
	"EDITBOX",   
	[
		"Coalesce Delay", 
		"How long the send thread waits, in microseconds, for additional messages to arrive before sending a partially-filled coalesced packet. Requires restart."
	], 
	"KH Networking", 
	"3000",
	1,
	{},
	true
] call CBA_fnc_addSetting;

[
	"KH_var_meleeDebugMode",
	"CHECKBOX",   
	[
		"Debug Mode", 
		"True displays debug information for melee actions."
	], 
	"KH Melee", 
	false,
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_meleePlayerAttackAnimationTypePriority",
	"LIST",   
	[
		"Player Attack Animation Type Priority", 
		"Defines whether absolute or additive animations are prioritized for player attacks. If an attack does not have a prioritized equivalent, the other one is used."
	], 
	[
        "KH Melee",
        "Animation Type Priorities"
    ], 
	[
        [0, 1],
        ["ABSOLUTE", "ADDITIVE"],
        0
    ],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_meleePlayerBlockAnimationTypePriority",
	"LIST",   
	[
		"Player Block Animation Type Priority", 
		"Defines whether absolute or additive animations are prioritized for player blocks. If a block does not have a prioritized equivalent, the other one is used."
	], 
	[
        "KH Melee",
        "Animation Type Priorities"
    ],
	[
        [0, 1],
        ["ABSOLUTE", "ADDITIVE"],
        0
    ],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_meleePlayerParryAnimationTypePriority",
	"LIST",   
	[
		"Player Parry Animation Type Priority", 
		"Defines whether absolute or additive animations are prioritized for player parries. If a parry does not have a prioritized equivalent, the other one is used."
	], 
	[
        "KH Melee",
        "Animation Type Priorities"
    ],
	[
        [0, 1],
        ["ABSOLUTE", "ADDITIVE"],
        0
    ],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_meleeAiAttackAnimationTypePriority",
	"LIST",   
	[
		"AI Attack Animation Type Priority", 
		"Defines whether absolute or additive animations are prioritized for AI attacks. If an attack does not have a prioritized equivalent, the other one is used."
	], 
	[
        "KH Melee",
        "Animation Type Priorities"
    ], 
	[
        [0, 1],
        ["ABSOLUTE", "ADDITIVE"],
        0
    ],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_meleeAiBlockAnimationTypePriority",
	"LIST",   
	[
		"AI Block Animation Type Priority", 
		"Defines whether absolute or additive animations are prioritized for AI blocks. If a block does not have a prioritized equivalent, the other one is used."
	], 
	[
        "KH Melee",
        "Animation Type Priorities"
    ],
	[
        [0, 1],
        ["ABSOLUTE", "ADDITIVE"],
        0
    ],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_meleeAiParryAnimationTypePriority",
	"LIST",   
	[
		"AI Parry Animation Type Priority", 
		"Defines whether absolute or additive animations are prioritized for AI parries. If a parry does not have a prioritized equivalent, the other one is used."
	], 
	[
        "KH Melee",
        "Animation Type Priorities"
    ],
	[
        [0, 1],
        ["ABSOLUTE", "ADDITIVE"],
        0
    ],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_meleeAbsoluteStaminaConsumptionMultiplier",
	"SLIDER",   
	[
		"Stamina Consumption Multiplier", 
		"Stamina consumption from all actions is multiplied by this value."
	], 
	[
        "KH Melee",
        "Stamina Multipliers"
    ], 
	[0.000, 10.000, 1.000, 3],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_meleeAbsoluteAttackStaminaConsumptionMultiplier",
	"SLIDER",   
	[
		"Attack Stamina Consumption Multiplier", 
		"Stamina consumption from attacks is multiplied by this value."
	], 
	[
        "KH Melee",
        "Stamina Multipliers"
    ], 
	[0.000, 10.000, 1.000, 3],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_meleeAbsoluteBlockStaminaConsumptionMultiplier",
	"SLIDER",   
	[
		"Block Stamina Consumption Multiplier", 
		"Stamina consumption from blocks is multiplied by this value."
	], 
	[
        "KH Melee",
        "Stamina Multipliers"
    ], 
	[0.000, 10.000, 1.000, 3],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_meleeAbsoluteParryStaminaConsumptionMultiplier",
	"SLIDER",   
	[
		"Parry Stamina Consumption Multiplier", 
		"Stamina consumption from parries is multiplied by this value."
	], 
	[
        "KH Melee",
        "Stamina Multipliers"
    ], 
	[0.000, 10.000, 1.000, 3],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_meleeAbsoluteKickStaminaConsumptionMultiplier",
	"SLIDER",   
	[
		"Kick Stamina Consumption Multiplier", 
		"Stamina consumption from kicks is multiplied by this value."
	], 
	[
        "KH Melee",
        "Stamina Multipliers"
    ], 
	[0.000, 10.000, 1.000, 3],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_meleeAbsoluteTackleStaminaConsumptionMultiplier",
	"SLIDER",   
	[
		"Tackle Stamina Consumption Multiplier", 
		"Stamina consumption from tackles is multiplied by this value."
	], 
	[
        "KH Melee",
        "Stamina Multipliers"
    ], 
	[0.000, 10.000, 1.000, 3],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_meleeAbsoluteDodgeStaminaConsumptionMultiplier",
	"SLIDER",   
	[
		"Dodge Stamina Consumption Multiplier", 
		"Stamina consumption from dodges is multiplied by this value."
	], 
	[
        "KH Melee",
        "Stamina Multipliers"
    ], 
	[0.000, 10.000, 1.000, 3],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_meleeAbsoluteStaminaExhaustionMultiplier",
	"SLIDER",   
	[
		"Stamina Exhaustion Multiplier", 
		"Stamina exhaustion from all incoming actions is multiplied by this value."
	], 
	[
        "KH Melee",
        "Stamina Multipliers"
    ], 
	[0.000, 10.000, 1.000, 3],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_meleeAbsoluteAttackStaminaExhaustionMultiplier",
	"SLIDER",   
	[
		"Attack Stamina Exhaustion Multiplier", 
		"Stamina exhaustion from incoming attacks is multiplied by this value."
	], 
	[
        "KH Melee",
        "Stamina Multipliers"
    ], 
	[0.000, 10.000, 1.000, 3],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_meleeAbsoluteBlockStaminaExhaustionMultiplier",
	"SLIDER",   
	[
		"Block Stamina Exhaustion Multiplier", 
		"Stamina exhaustion from incoming blocks is multiplied by this value."
	], 
	[
        "KH Melee",
        "Stamina Multipliers"
    ], 
	[0.000, 10.000, 1.000, 3],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_meleeAbsoluteParryStaminaExhaustionMultiplier",
	"SLIDER",   
	[
		"Parry Stamina Exhaustion Multiplier", 
		"Stamina exhaustion from incoming parries is multiplied by this value."
	], 
	[
        "KH Melee",
        "Stamina Multipliers"
    ], 
	[0.000, 10.000, 1.000, 3],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_meleeAbsoluteKickStaminaExhaustionMultiplier",
	"SLIDER",   
	[
		"Kick Stamina Exhaustion Multiplier", 
		"Stamina exhaustion from incoming kicks is multiplied by this value."
	], 
	[
        "KH Melee",
        "Stamina Multipliers"
    ], 
	[0.000, 10.000, 1.000, 3],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_meleeAbsoluteTackleStaminaExhaustionMultiplier",
	"SLIDER",   
	[
		"Tackle Stamina Exhaustion Multiplier", 
		"Stamina exhaustion from incoming tackles is multiplied by this value."
	], 
	[
        "KH Melee",
        "Stamina Multipliers"
    ], 
	[0.000, 10.000, 1.000, 3],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_meleeAbsoluteTackleBlockStaminaExhaustionMultiplier",
	"SLIDER",   
	[
		"Tackle Block Stamina Exhaustion Multiplier", 
		"Stamina exhaustion from incoming tackles while blocking is multiplied by this value."
	], 
	[
        "KH Melee",
        "Stamina Multipliers"
    ], 
	[0.000, 10.000, 1.000, 3],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_meleeAttacksInterruptAttacks",
	"CHECKBOX",   
	[
		"Attacks Interrupt Attacks", 
		"True makes it so that attacks can be interrupted by powerful attacks."
	], 
	[
        "KH Melee",
        "General"
    ], 
	true,
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_meleeAttacksInterruptKicks",
	"CHECKBOX",   
	[
		"Attacks Interrupt Kicks", 
		"True makes it so that kicks can be interrupted by powerful attacks."
	], 
	[
        "KH Melee",
        "General"
    ], 
	true,
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_meleeAttacksInterruptTackles",
	"CHECKBOX",   
	[
		"Attacks Interrupt Tackles", 
		"True makes it so that tackles can be interrupted by powerful attacks."
	], 
	[
        "KH Melee",
        "General"
    ], 
	true,
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_meleeAttackBlockOnInsufficientStamina",
	"CHECKBOX",   
	[
		"Attack Block On Insufficient Stamina", 
		"True results in all attacks being blocked if the amount of stamina required for the attack is insufficient. Damage still passes through, but the combo is interrupted."
	], 
	[
        "KH Melee",
        "General"
    ], 
	true,
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_meleeAttackIgnoreFriendlies",
	"CHECKBOX",   
	[
		"Attack Ignore Friendlies", 
		"True results in attack hit detection ignoring friendlies."
	], 
	[
        "KH Melee",
        "General"
    ], 
	false,
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_meleeKickIgnoreFriendlies",
	"CHECKBOX",   
	[
		"Kick Ignore Friendlies", 
		"True results in kick hit detection ignoring friendlies."
	], 
	[
        "KH Melee",
        "General"
    ], 
	false,
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_meleeTackleIgnoreFriendlies",
	"CHECKBOX",   
	[
		"Tackle Ignore Friendlies", 
		"True results in tackle hit detection ignoring friendlies."
	], 
	[
        "KH Melee",
        "General"
    ], 
	false,
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_meleeAiCollisionDetection",
	"CHECKBOX",   
	[
		"AI Collision Detection", 
		"True enables a system that tries its best to avoid AI clipping into other AI during melee actions."
	], 
	[
        "KH Melee",
        "General"
    ], 
	true,
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_meleePowerReductionStaminaCoefficient",
	"SLIDER",   
	[
		"Power Reduction Stamina Coefficient", 
		"Power for all melee actions is reduced by this value as a percentage relative to the remaining stamina. 1.000 is 100% power, 0 is 0% power. For example, a value of 0.5 will cut power by half if stamina is completely exhausted."
	], 
	[
        "KH Melee",
        "General"
    ], 
	[0.000, 1.000, 0.000, 3],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_meleeDamageReductionStaminaCoefficient",
	"SLIDER",   
	[
		"Damage Reduction Stamina Coefficient", 
		"Damage for all melee actions is reduced by this value as a percentage relative to the remaining stamina. 1.000 is 100% damage, 0 is 0% damage. For example, a value of 0.5 will cut damage by half if stamina is completely exhausted."
	], 
	[
        "KH Melee",
        "General"
    ], 
	[0.000, 1.000, 0.000, 3],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_meleeDodgeFailureAngleRange",
	"SLIDER",   
	[
		"Dodge Failure Angle Range", 
		"Dodges that move towards a unit who is performing an offensive action will fail if said unit is within the number of degrees defined here, relative to the direction of the dodge."
	], 
	[
        "KH Melee",
        "General"
    ], 
	[0, 360, 90, 0],
	1,
	{
		[
			[],
			{
				KH_var_meleeDodgeFailureAngleRange = KH_var_meleeDodgeFailureAngleRange / 2;
				publicVariable "KH_var_meleeDodgeFailureAngleRange";
			},
			"SERVER",
			"1",
			false
		] call KH_fnc_execute;
	},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_meleeAiEngageDistance",
	"SLIDER",   
	[
		"AI Engage Distance", 
		"Distance within which melee AI will try to engage enemy units, in metres."
	], 
	[
        "KH Melee",
        "General"
    ], 
	[0, 1000, 100, 0],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH Melee", 
	"KH_meleeCycleAttackMode", 
	"Cycle Attack Mode",
	{
		if (!dialog && !visibleMap && (isNull curatorCamera) && (KH_var_playerUnit getVariable ["KH_var_inMeleeState", false])) then {
			[KH_var_playerUnit, "CYCLE_ATTACK_MODE"] call KH_fnc_updateMeleeState;
		};
	}, 
	{}, 
	[0x21, [false, false, false]]
] call CBA_fnc_addKeybind;

[
	"KH Melee", 
	"KH_meleeDodge", 
	"Dodge",
	{
		if (!dialog && !visibleMap && (isNull curatorCamera) && (KH_var_playerUnit getVariable ["KH_var_inMeleeState", false])) then {
			[KH_var_playerUnit, "DODGE"] call KH_fnc_updateMeleeState;
		};
	}, 
	{}, 
	[0x39, [false, false, false]]
] call CBA_fnc_addKeybind;

[
	"KH Melee", 
	"KH_meleeBlock", 
	"Block",
	{
		if (!dialog && !visibleMap && (isNull curatorCamera) && (KH_var_playerUnit getVariable ["KH_var_inMeleeState", false])) then {
			[KH_var_playerUnit, "BLOCK_IN"] call KH_fnc_updateMeleeState;
		};
	},
	{
		if (!dialog && !visibleMap && (isNull curatorCamera) && (KH_var_playerUnit getVariable ["KH_var_inMeleeState", false])) then {
			[KH_var_playerUnit, "BLOCK_OUT"] call KH_fnc_updateMeleeState;
		};
	},
	[0x10, [false, false, false]]
] call CBA_fnc_addKeybind;

[
	"KH Melee", 
	"KH_meleeParry", 
	"Parry",
	{
		if (!dialog && !visibleMap && (isNull curatorCamera) && (KH_var_playerUnit getVariable ["KH_var_inMeleeState", false])) then {
			[KH_var_playerUnit, "PARRY"] call KH_fnc_updateMeleeState;
		};
	}, 
	{}, 
	[0x12, [false, false, false]]
] call CBA_fnc_addKeybind;

[
	"KH Melee", 
	"KH_meleeKick", 
	"Kick",
	{
		if (!dialog && !visibleMap && (isNull curatorCamera) && (KH_var_playerUnit getVariable ["KH_var_inMeleeState", false])) then {
			[KH_var_playerUnit, "KICK"] call KH_fnc_updateMeleeState;
		};
	}, 
	{}, 
	[0x13, [false, false, false]]
] call CBA_fnc_addKeybind;

[
	"KH Melee", 
	"KH_meleeTackle", 
	"Tackle",
	{
		if (!dialog && !visibleMap && (isNull curatorCamera) && (KH_var_playerUnit getVariable ["KH_var_inMeleeState", false])) then {
			[KH_var_playerUnit, "TACKLE"] call KH_fnc_updateMeleeState;
		};
	}, 
	{}, 
	[0x2D, [false, false, false]]
] call CBA_fnc_addKeybind;

[
	"KH Miscellaneous", 
	"KH_speechRecognitionToggle", 
	"Speech Recognition (TOGGLE)",
	{
		if sttIsInitialized then {
			if sttIsCapturing then {
				sttStopCapture;

				if !(uiNamespace isNil "KH_var_speechRecognitionDisplay") then {
					ctrlDelete (uiNamespace getVariable "KH_var_speechRecognitionDisplay");
					uiNamespace setVariable ["KH_var_speechRecognitionDisplay", nil];
				};
			}
			else {
				sttStartCapture;

				if (uiNamespace isNil "KH_var_speechRecognitionDisplay") then {
					uiNamespace setVariable [
						"KH_var_speechRecognitionDisplay",
						["RscText", "CAPTURING", [0, false, 0], [0, 0, 0, 0], [0, 0, 10, 2], false, [0, 0, 0]] call KH_fnc_draw2d
					];
				};
			};

			if (missionNamespace isNil "KH_var_sttHandler") then {
				missionNamespace setVariable [
					"KH_var_sttHandler",
					[
						[],
						{
							if sttIsInitialized then {
								if !sttIsCapturing then {
									if !(uiNamespace isNil "KH_var_speechRecognitionDisplay") then {
										ctrlDelete (uiNamespace getVariable "KH_var_speechRecognitionDisplay");
										uiNamespace setVariable ["KH_var_speechRecognitionDisplay", nil];
									};
								};
							};
						},
						true,
						0,
						false
					] call KH_fnc_execute
				];
			};
		};
	}, 
	{}, 
	[0xDB, [false, false, true]]
] call CBA_fnc_addKeybind;

[
	"KH Miscellaneous", 
	"KH_speechRecognitionHold", 
	"Speech Recognition (HOLD)",
	{
		if sttIsInitialized then {
			if !sttIsCapturing then {
				sttStartCapture;

				if (uiNamespace isNil "KH_var_speechRecognitionDisplay") then {
					uiNamespace setVariable [
						"KH_var_speechRecognitionDisplay",
						["RscText", "CAPTURING", [0, false, 0], [0, 0, 0, 0], [0, 0, 10, 2], false, [0, 0, 0]] call KH_fnc_draw2d
					];
				};
			};

			if (missionNamespace isNil "KH_var_sttHandler") then {
				missionNamespace setVariable [
					"KH_var_sttHandler",
					[
						[],
						{
							if sttIsInitialized then {
								if !sttIsCapturing then {
									if !(uiNamespace isNil "KH_var_speechRecognitionDisplay") then {
										ctrlDelete (uiNamespace getVariable "KH_var_speechRecognitionDisplay");
										uiNamespace setVariable ["KH_var_speechRecognitionDisplay", nil];
									};
								};
							};
						},
						true,
						0,
						false
					] call KH_fnc_execute
				];
			};
		};
	}, 
	{
		if sttIsInitialized then {
			if sttIsCapturing then {
				sttStopCapture;

				if !(uiNamespace isNil "KH_var_speechRecognitionDisplay") then {
					ctrlDelete (uiNamespace getVariable "KH_var_speechRecognitionDisplay");
					uiNamespace setVariable ["KH_var_speechRecognitionDisplay", nil];
				};
			};
		};
	}, 
	[0x0F, [false, false, false]]
] call CBA_fnc_addKeybind;

[
	"KH Miscellaneous", 
	"KH_toggleDiagnostics", 
	"Toggle Diagnostics",
	{
		if (KH_var_adminMachine isEqualTo clientOwner) then {
			[!KH_var_diagnosticsState] call KH_fnc_diagnostics;
		};
	}, 
	{}, 
	[0xC7, [false, false, false]]
] call CBA_fnc_addKeybind;

[
	"KH Miscellaneous", 
	"KH_console", 
	"Console",
	{
		if (KH_var_adminMachine isEqualTo clientOwner) then {
			private _display = createDialog ["KH_Console", true];
			private _currentConsoleCache = profileNamespace getVariable ["KH_var_consoleCache", []];
			ctrlSetText [101, _currentConsoleCache param [((count _currentConsoleCache) -1) max 0, ""]];
			lbSetCurSel [108, profileNamespace getVariable ["KH_var_consoleLanguage", 0]];
			KH_var_consoleCacheIndex = ((count _currentConsoleCache) - 1) max 0;
			lbClear 109;
			lbAdd [109, "None"];

			{
				lbAdd [109, _x];
			} forEach ([KH_var_quickFunctionsSqf, KH_var_quickFunctionsLua] select (lbCurSel 108));

			lbSetCurSel [109, 0];

			[
				["CONTROL", _display displayCtrl 104],
				"ButtonClick",
				[],
				{
					private _input = ctrlText 101;

					if (_input isNotEqualTo "") then {
						private _currentConsoleCache = profileNamespace getVariable ["KH_var_consoleCache", []];

						if (((count _input) <= 8192) && ((_currentConsoleCache param [((count _currentConsoleCache) - 1) max 0, ""]) isNotEqualTo _input)) then {
							_currentConsoleCache pushBack _input;

							if ((count _currentConsoleCache) > 32) then {
								_currentConsoleCache deleteAt 0;
							};

							profileNamespace setVariable ["KH_var_consoleCache", _currentConsoleCache];
						};

						ctrlSetText [
							103, 
							[
								if ((lbCurSel 108) isEqualTo 0) then {
									call (compile _input);
								}
								else {
									luaExecute _input;
								}
							] joinString ""
						];

						KH_var_consoleCacheIndex = ((count _currentConsoleCache) - 1) max 0;
					};

					nil;
				}
			] call KH_fnc_addEventHandler;

			[
				["CONTROL", _display displayCtrl 105],
				"ButtonClick",
				[],
				{
					private _input = ctrlText 101;
					
					if (_input isNotEqualTo "") then {
						private _currentConsoleCache = profileNamespace getVariable ["KH_var_consoleCache", []];

						if (((count _input) <= 8192) && ((_currentConsoleCache param [((count _currentConsoleCache) - 1) max 0, ""]) isNotEqualTo _input)) then {
							_currentConsoleCache pushBack _input;

							if ((count _currentConsoleCache) > 32) then {
								_currentConsoleCache deleteAt 0;
							};

							profileNamespace setVariable ["KH_var_consoleCache", _currentConsoleCache];
						};

						ctrlSetText [
							103, 
							[
								if ((lbCurSel 108) isEqualTo 0) then {
									diag_codePerformance [compile _input, [], 10000];
								}
								else {
									luaExecute ([
										"return util.profile(10000, function() ",
										_input,
										" end)"	
									] joinString "");
								}
							] joinString ""
						];

						KH_var_consoleCacheIndex = ((count _currentConsoleCache) - 1) max 0;
					};

					nil;
				}
			] call KH_fnc_addEventHandler;

			[
				["DISPLAY", _display],
				"KeyDown",
				[],
				{
					(_this select [1]) params ["_key", "_shift"];

					if ((_key isEqualTo 0x1C) && !_shift) then {
						private _input = ctrlText 101;

						if (_input isNotEqualTo "") then {
							private _currentConsoleCache = profileNamespace getVariable ["KH_var_consoleCache", []];

							if (((count _input) <= 8192) && ((_currentConsoleCache param [((count _currentConsoleCache) - 1) max 0, ""]) isNotEqualTo _input)) then {
								_currentConsoleCache pushBack _input;

								if ((count _currentConsoleCache) > 32) then {
									_currentConsoleCache deleteAt 0;
								};

								profileNamespace setVariable ["KH_var_consoleCache", _currentConsoleCache];
							};

							ctrlSetText [
								103, 
								[
									if ((lbCurSel 108) isEqualTo 0) then {
										call (compile _input);
									}
									else {
										luaExecute _input;
									}
								] joinString ""
							];

							KH_var_consoleCacheIndex = ((count _currentConsoleCache) - 1) max 0;
						};

						true;
					};
				}
			] call KH_fnc_addEventHandler;

			[
				["CONTROL", _display displayCtrl 106],
				"ButtonClick",
				[],
				{
					KH_var_consoleCacheIndex = (KH_var_consoleCacheIndex - 1) max 0;
					ctrlSetText [101, (profileNamespace getVariable ["KH_var_consoleCache", []]) param [KH_var_consoleCacheIndex, ""]];
				}
			] call KH_fnc_addEventHandler;

			[
				["CONTROL", _display displayCtrl 107],
				"ButtonClick",
				[],
				{
					private _currentConsoleCache = profileNamespace getVariable ["KH_var_consoleCache", []];
					KH_var_consoleCacheIndex = (KH_var_consoleCacheIndex + 1) min ((count _currentConsoleCache) - 1);
					ctrlSetText [101, _currentConsoleCache param [KH_var_consoleCacheIndex, ""]];
				}
			] call KH_fnc_addEventHandler;

			[
				["CONTROL", _display displayCtrl 108],
				"ToolBoxSelChanged",
				[],
				{
					private _selectedIndex = param [1, 0];
					profileNamespace setVariable ["KH_var_consoleLanguage", _selectedIndex];
					lbClear 109;
					lbAdd [109, "None"];

					{
						lbAdd [109, _x];
					} forEach ([KH_var_quickFunctionsSqf, KH_var_quickFunctionsLua] select _selectedIndex);

					lbSetCurSel [109, 0];
				}
			] call KH_fnc_addEventHandler;

			[
				["CONTROL", _display displayCtrl 109],
				"LBSelChanged",
				[],
				{
					private _selectedIndex = param [1, 0];

					if (_selectedIndex isNotEqualTo 0) then {
						private _language = lbCurSel 109;
						private _entry = ([KH_var_quickFunctionsSqf, KH_var_quickFunctionsLua] select _language) get (lbText [109, _selectedIndex]);

						if !(isNil "_entry") then {
							if (_language isEqualTo 0) then {
								ctrlSetText [101, toString _entry];
							}
							else {
								ctrlSetText [101, _entry];
							};
						};
					};
				}
			] call KH_fnc_addEventHandler;
		};
	}, 
	{}, 
	[0xD2, [false, false, false]]
] call CBA_fnc_addKeybind;