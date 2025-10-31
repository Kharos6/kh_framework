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
	"KH_var_playingDeadStartDuration",
	"TIME",   
	[
		"Playing Dead Start Duration", 
		"Time it takes for a player who is playing dead to be turned into a captive to prevent enemies from shooting at them."
	], 
	[
        "KH Miscellaneous",
        "Deception"
    ], 
	[0, 60, 5],
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
		"The amount of damage to total health beyond which a unit will be incapacitated. 1.00 is 100% health, 0.00 is 0% health."
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
	"KH_var_incapacitatedCaptives",
	"CHECKBOX",   
	[
		"Incapacitated Captives", 
		"True sets incapacitated units as captives so that their enemies avoid shooting them while they are incapacitated. The captivity status is reset upon revive or withstanding, unless the revive was conducted by an enemy unit."
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
	false,
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_allowWoundPressing",
	"CHECKBOX",   
	[
		"Wound Pressing", 
		"True allows players to press the wound of an incapacitated unit in order to slow down the Incapacitation Death Time."
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
	"KH_var_woundPressMultiplier",
	"SLIDER",   
	[
		"Would Press Multiplier", 
		"The current remaining Incapacitation Death Time tick speed is multiplied by this value when the wound is being pressed."
	], 
	[
        "KH Medical",
        "Incapacitation"
    ], 
	[0, 1.00, 0.50, 2],
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
	[0, 300, 30],
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
	[0, 300, 15],
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
	[0, 300, 45],
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
	[0, 300, 30],
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
		"Amount of first aid kits required for people to revive themselves when no medikit is available and Self Revive Require Medikit is false."
	], 
	[
        "KH Medical",
        "Incapacitation"
    ], 
	[0, 15, 4, 0],
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
	false,
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_maximumReviveHeal",
	"SLIDER",   
	[
		"Maximum Revive Heal",
		"Amount of health that is recovered after reviving, normalized to Incapacitation Threshold. 1.00 is 100% health, 0.00 is the incapacitation threshold health."
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
	"KH_var_maximumReviveHealMedic",
	"SLIDER",   
	[
		"Maximum Revive Heal medic",
		"Amount of health that is recovered by medics after reviving, normalized to Incapacitation Threshold. 1.00 is 100% health, 0.00 is the incapacitation threshold health."
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
	"KH_var_withstandThreshold",
	"TIME",   
	[
		"Withstand Threshold", 
		"Time required for a player to be incapacitated before they are able to withstand."
	], 
	[
        "KH Medical",
        "Withstanding"
    ], 
	[0, 1800, 15],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_withstandActionDuration",
	"TIME",   
	[
		"Withstand Action Duration", 
		"The time it takes to withstand."
	], 
	[
        "KH Medical",
        "Withstanding"
    ], 
	[0, 60, 5],
	1,
	{},
	false
] call CBA_fnc_addSetting;

[
	"KH_var_withstandDuration",
	"TIME",   
	[
		"Withstand Duration", 
		"Time it takes for a withstanding player to collapse into the incapacitation state again, with no more withstanding ability until they are revived. Set to 0 for infinite withstanding."
	], 
	[
        "KH Medical",
        "Withstanding"
    ], 
	[0, 1800, 60],
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
	false,
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
	[0.00, 10.00, 1.00, 2],
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
	[0.00, 10.00, 1.00, 2],
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
	[0.00, 10.00, 1.00, 2],
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
	[0.00, 10.00, 1.00, 2],
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
	[0.00, 10.00, 1.00, 2],
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
	[0.00, 10.00, 1.00, 2],
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
	[0.00, 10.00, 1.00, 2],
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
	[0.00, 10.00, 1.00, 2],
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
	[0.00, 10.00, 1.00, 2],
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
	[0.00, 10.00, 1.00, 2],
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
	[0.00, 10.00, 1.00, 2],
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
	[0.00, 10.00, 0.50, 2],
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
	[0.00, 10.00, 2.00, 2],
	1,
	{},
	false
] call CBA_fnc_addSetting;