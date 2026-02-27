class CfgKHMeleeTypes
{
    class KH_MeleeGenericMan
    {
        class Attack01
        {
            type = "KH_MeleeGenericManSwing";
            damageFunction = "";
            radius = 0.45;
            blockPower = 0.1;
            parryPower = 0.1;
            kickPower = 0.5;
            tacklePower = 0.5;
            cost = 0.1;
            costInfliction = 0.05;
            class Sounds
            {
                swing[] = {};
                thrust[] = {};
                generic[] = {};
                blocked[] = {};
                parried[] = {};
                flesh[] = {};
            };
        };
        class Attack02
        {
            type = "KH_MeleeGenericManThrust";
            damageFunction = "";
            radius = 0.45;
            blockPower = 0.1;
            parryPower = 0.1;
            kickPower = 0.5;
            tacklePower = 0.5;
            cost = 0.1;
            costInfliction = 0.05;
            class Sounds
            {
                swing[] = {};
                thrust[] = {};
                generic[] = {};
                blocked[] = {};
                parried[] = {};
                flesh[] = {};
            };
        };
        class Kick01
        {
            type = "KH_MeleeGenericManKick";
            damageFunction = "";
            power = 0.5;
            cost = 0.15;
            costInfliction = 0.2;
            class Sounds
            {
                generic[] = {"KH_MeleeKick01", "KH_MeleeKick02", "KH_MeleeKick03", "KH_MeleeKick04"};
                flesh[] = {"KH_MeleeKick01", "KH_MeleeKick02", "KH_MeleeKick03", "KH_MeleeKick04"};
            };
        };
        class Tackle01
        {
            type = "KH_MeleeGenericManTackle";
            damageFunction = "";
            angle = 45;
            power = 0.5;
            cost = 0.2;
            costInfliction = 0.25;
            costBlockInfliction = 0.75;
            class Sounds
            {
                generic[] = {"KH_MeleeTackle01", "KH_MeleeTackle02"};
                flesh[] = {"KH_MeleeTackle01", "KH_MeleeTackle02"};
            };
        };
        class Block01
        {
            angle = 45;
            power = 0.1;
            cost = 0.25;
            costInfliction = 0.1;
        };
        class Parry01
        {
            angle = 45;
            power = 0.1;
            cost = 0.1;
            costInfliction = 0.4;
        };
    };
    class KH_MeleeCleavingSharpLightLong: KH_MeleeGenericMan
    {
        class Attack01: Attack01
        {
            type = "KH_MeleeCleavingSharpLightLongSwing";
            blockPower = 0.15;
            parryPower = 0.15;
            class Sounds
            {
                swing[] = {"KH_MeleeSwingLight01", "KH_MeleeSwingLight02", "KH_MeleeSwingLight03", "KH_MeleeSwingLight04", "KH_MeleeSwingLight05", "KH_MeleeSwingLight06"};
                thrust[] = {"KH_MeleeSwingLight01", "KH_MeleeSwingLight02", "KH_MeleeSwingLight03", "KH_MeleeSwingLight04", "KH_MeleeSwingLight05", "KH_MeleeSwingLight06"};
                generic[] = {"KH_MeleeSharpHitMetal01", "KH_MeleeSharpHitMetal02", "KH_MeleeSharpHitMetal03"};
                blocked[] = {"KH_MeleeSharpHitMetal01", "KH_MeleeSharpHitMetal02", "KH_MeleeSharpHitMetal03"};
                parried[] = {"KH_MeleeSharpScrape01", "KH_MeleeSharpScrape02"};
                flesh[] = {"KH_MeleeSharpHitFlesh01", "KH_MeleeSharpHitFlesh02", "KH_MeleeSharpHitFlesh03", "KH_MeleeSharpHitFlesh04", "KH_MeleeSharpHitFlesh05"};
            };
        };
        class Attack02: Attack02
        {
            type = "KH_MeleeCleavingSharpLightLongThrust";
            blockPower = 0.15;
            parryPower = 0.15;
            class Sounds
            {
                swing[] = {"KH_MeleeSwingLight01", "KH_MeleeSwingLight02", "KH_MeleeSwingLight03", "KH_MeleeSwingLight04", "KH_MeleeSwingLight05", "KH_MeleeSwingLight06"};
                thrust[] = {"KH_MeleeSwingLight01", "KH_MeleeSwingLight02", "KH_MeleeSwingLight03", "KH_MeleeSwingLight04", "KH_MeleeSwingLight05", "KH_MeleeSwingLight06"};
                generic[] = {"KH_MeleeSharpHitMetal01", "KH_MeleeSharpHitMetal02", "KH_MeleeSharpHitMetal03"};
                blocked[] = {"KH_MeleeSharpHitMetal01", "KH_MeleeSharpHitMetal02", "KH_MeleeSharpHitMetal03"};
                parried[] = {"KH_MeleeSharpScrape01", "KH_MeleeSharpScrape02"};
                flesh[] = {"KH_MeleeSharpHitFlesh01", "KH_MeleeSharpHitFlesh02", "KH_MeleeSharpHitFlesh03", "KH_MeleeSharpHitFlesh04", "KH_MeleeSharpHitFlesh05"};
            };
        };
        class Block01: Block01
        {
            power = 0.2;
        };
        class Parry01: Parry01
        {
            power = 0.25;
        };
    };
    class KH_MeleeCleavingSharpLightShort: KH_MeleeGenericMan
    {
        class Attack01: Attack01
        {
            type = "KH_MeleeCleavingSharpLightShortSwing";
            blockPower = 0.15;
            parryPower = 0.15;
            class Sounds
            {
                swing[] = {"KH_MeleeSwingLight01", "KH_MeleeSwingLight02", "KH_MeleeSwingLight03", "KH_MeleeSwingLight04", "KH_MeleeSwingLight05", "KH_MeleeSwingLight06"};
                thrust[] = {};
                generic[] = {"KH_MeleeSharpHitMetal01", "KH_MeleeSharpHitMetal02", "KH_MeleeSharpHitMetal03"};
                blocked[] = {"KH_MeleeSharpHitMetal01", "KH_MeleeSharpHitMetal02", "KH_MeleeSharpHitMetal03"};
                parried[] = {"KH_MeleeSharpScrape01", "KH_MeleeSharpScrape02"};
                flesh[] = {"KH_MeleeSharpHitFlesh01", "KH_MeleeSharpHitFlesh02", "KH_MeleeSharpHitFlesh03", "KH_MeleeSharpHitFlesh04", "KH_MeleeSharpHitFlesh05"};
            };
        };
        class Attack02: Attack02
        {
            type = "KH_MeleeCleavingSharpLightShortThrust";
            blockPower = 0.1;
            parryPower = 0.1;
            class Sounds
            {
                swing[] = {"KH_MeleeSwingLight01", "KH_MeleeSwingLight02", "KH_MeleeSwingLight03", "KH_MeleeSwingLight04", "KH_MeleeSwingLight05", "KH_MeleeSwingLight06"};
                thrust[] = {};
                generic[] = {"KH_MeleeSharpHitMetal01", "KH_MeleeSharpHitMetal02", "KH_MeleeSharpHitMetal03"};
                blocked[] = {"KH_MeleeSharpHitMetal01", "KH_MeleeSharpHitMetal02", "KH_MeleeSharpHitMetal03"};
                parried[] = {"KH_MeleeSharpScrape01", "KH_MeleeSharpScrape02"};
                flesh[] = {"KH_MeleeSharpHitFlesh01", "KH_MeleeSharpHitFlesh02", "KH_MeleeSharpHitFlesh03", "KH_MeleeSharpHitFlesh04", "KH_MeleeSharpHitFlesh05"};
            };
        };
        class Block01: Block01
        {
            power = 0.1;
        };
        class Parry01: Parry01
        {
            power = 0.2;
        };
    };
};