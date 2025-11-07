[
	"KH_var_playingDeadAllowed",
	"CHECKBOX",
	[
		"Play Dead", 
		"True allows players to play dead if they are not currently a captive."
	], 
	[
        "KH Miscellaneous",
        "Deception"
    ],
	false,
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_playingDeadDiscoveryDistanceMinimum",
	"SLIDER",   
	[
		"Playing Dead Discovery Distance Minimum", 
		"Distance in metres within which a player who is playing dead will instantly be discovered."
	], 
	[
        "KH Miscellaneous",
        "Deception"
    ], 
	[0, 100, 2, 0],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_playingDeadDiscoveryDistanceMaximum",
	"SLIDER",   
	[
		"Playing Dead Discovery Distance Maximum", 
		"Distance in metres within which a player who is playing dead may be discovered. The chance of discovery ticks every second. The chance is 100% at minimum distance and below, and 0% at maximum distance and above."
	], 
	[
        "KH Miscellaneous",
        "Deception"
    ], 
	[0, 100, 100, 0],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_playingDeadStartDelay",
	"TIME",   
	[
		"Playing Dead Start Delay", 
		"Time it takes for a player who is playing dead to be turned into a captive to prevent enemies from shooting at them."
	], 
	[
        "KH Miscellaneous",
        "Deception"
    ], 
	[0, 60, 3],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_khMedical",
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
	{},
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
	[0, 1.00, 0.90, 2],
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
		"True only allows medics to perform revives on other people."
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
		"True makes the medikit required for reviving other people."
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
		"Amount of first aid kits required for reviving other people when no medikit is available and Revive Require Medikit is false."
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
	"KH_var_selfRevive",
	"CHECKBOX",   
	[
		"Self Revive", 
		"True allows people to revive themselves."
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
		"True makes the medikit required for people to revive themselves."
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
		"Amount of first aid kits required for a person to revive themselves when no medikit is available and Revive Require Medikit is false."
	], 
	[
        "KH Medical",
        "Incapacitation"
    ], 
	[0, 15, 6, 0],
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
	[0, 1.00, 0.50, 2],
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
	[0, 1.00, 0.50, 2],
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
	[0, 1.00, 1.00, 2],
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
	[0, 1.00, 1.00, 2],
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
	[0, 1.00, 0.25, 2],
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
	[0, 1.00, 0.25, 2],
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
	[0, 1.00, 0.50, 2],
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
	[0, 1.00, 0.50, 2],
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
	[0, 1.00, 0.50, 2],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_reviveHealMedic",
	"SLIDER",   
	[
		"Revive Heal medic",
		"Amount of health that is recovered by medics after reviving. 1.00 is 100% health, 0.00 is 0% health. If the amount of health results in the remaining damage being above the incapacitation threshold, the next hit on the revived unit will immediately incapacitate them."
	], 
	[
        "KH Medical",
        "Healing"
    ], 
	[0, 1.00, 1.00, 2],
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
	[0, 1.00, 0.50, 2],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_selfReviveHealMedic",
	"SLIDER",   
	[
		"Self Revive Heal medic",
		"Amount of health that is recovered by medics after they revive themselves. 1.00 is 100% health, 0.00 is 0% health. If the amount of health results in the remaining damage being above the incapacitation threshold, the next hit on the revived unit will immediately incapacitate them."
	], 
	[
        "KH Medical",
        "Healing"
    ], 
	[0, 1.00, 1.00, 2],
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
		"Medic Only", 
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
		"Require Medikit", 
		"True makes it so that a medikit is required to stabilize a person."
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
		"Required First Aid Kits", 
		"Amount of first aid kits required for stabilizing a person when no medikit is available and Require Medikit is false."
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
	[0.00, 10.000, 1.000, 3],
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
	[0.00, 10.000, 1.000, 3],
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
	[0.00, 10.000, 1.000, 3],
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
	[0.00, 10.000, 1.000, 3],
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
	[0.00, 10.000, 1.000, 3],
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
	[0.00, 10.000, 1.000, 3],
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
	[0.00, 10.000, 1.000, 3],
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
	[0.00, 10.000, 1.000, 3],
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
	[0.00, 10.000, 1.000, 3],
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
	[0.00, 10.000, 1.000, 3],
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
	[0.00, 10.000, 1.000, 3],
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
	[0.00, 10.000, 0.500, 3],
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
	[0.00, 10.000, 2.000, 3],
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
	[0.00, 100.00, 40.00, 2],
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
	[0.00, 100.00, 46.50, 2],
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
	[0.00, 100.00, 94.00, 2],
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
	0,
	{},
	false
] call CBA_fnc_addSetting;