class CfgKHMeleeTypes
{
    class KH_MeleeGenericMan
    {
        class Hit01
        {
            type = "KH_MeleeGenericManSwing";
            damageFunction = "";
            radius = 0.25;
            blockPower = 0.1;
            parryPower = 0.1;
            cost = 0.1;
        };
        class Hit02
        {
            type = "KH_MeleeGenericManThrust";
            damageFunction = "";
            radius = 0.25;
            blockPower = 0.1;
            parryPower = 0.1;
            cost = 0.1;
        };
        class Kick01
        {
            type = "KH_MeleeGenericManKick";
            damageFunction = "";
            power = 0.1;
            cost = 0.15;
        };
        class Tackle01
        {
            type = "KH_MeleeGenericManTackle";
            damageFunction = "";
            angle = 45;
            power = 0.1;
            cost = 0.2;
        };
        class Block01
        {
            angle = 45;
            power = 0.1;
            cost = 0.05;
        };
        class Parry01
        {
            angle = 45;
            power = 0.1;
            cost = 0.1;
        };
    };
    class KH_MeleeCleavingSharpLight: KH_MeleeGenericMan
    {
        class Hit01: Hit01
        {
            type = "KH_MeleeCleavingSharpLightSwing";
            blockPower = 0.15;
            parryPower = 0.15;
        };
        class Hit02: Hit02
        {
            type = "KH_MeleeCleavingSharpLightThrust";
            blockPower = 0.15;
            parryPower = 0.15;
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
    class KH_MeleeGenericBluntLight: KH_MeleeGenericMan
    {
        class Hit01: Hit01
        {
            type = "KH_MeleeBluntLightSwing";
            blockPower = 0.15;
            parryPower = 0.15;
        };
        class Hit02: Hit02
        {
            type = "KH_MeleeBluntLightThrust";
            blockPower = 0.15;
            parryPower = 0.15;
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
};