class CfgMovesBasic
{
    class Actions;
    class Default
    {
        kh_melee = 0;
        kh_meleeHitTiming[] = {};
        kh_meleeBlockTiming[] = {};
        kh_meleeParryTiming[] = {};
        kh_meleeKickTiming[] = {};
        kh_meleeTackleTiming[] = {};
        kh_meleeDodgeTiming[] = {};
        kh_meleeSoundTiming[] = {};
    };
    class ManActions
    {
        KH_MeleeDodge = "";
        KH_MeleeInLightSword = "Hum_AbsStaSaf_IdlLhtMel";
        KH_MeleeInLightSwordTransition = "Hum_AbsStaSaf_IdlUna_Tra_AbsStaSaf_IdlLhtMel";
        KH_MeleeBlockGesture[] = {"", "Gesture"};
        KH_MeleeBlockIn = "";
        KH_MeleeBlockOut = "";
        KH_MeleeBlockSuccess = "";
        KH_MeleeBlockSuccessGesture[] = {"", "Gesture"};
        KH_MeleeBlockFailure = "";
        KH_MeleeParry = "";
        KH_MeleeParryGesture[] = {"", "Gesture"};
        KH_MeleeKick = "";
        KH_MeleeTackle = "";
        KH_MeleeStop = "";
        KH_MeleeRecoil = "";
        KH_Attack01 = "";
        KH_Attack02 = "";
        KH_Attack01Gesture[] = {"", "Gesture"};
        KH_Attack02Gesture[] = {"", "Gesture"};
        KH_StaggerLightForward = "";
        KH_StaggerLightBackward = "";
        KH_StaggerLightLeftward = "";
        KH_StaggerLightRightward = "";
        KH_StaggerHeavyForward = "";
        KH_StaggerHeavyBackward = "";
        KH_StaggerHeavyLeftward = "";
        KH_StaggerHeavyRightward = "";
        KH_MeleeHitLeftGesture[] = {"", "Gesture"};
        KH_MeleeHitRightGesture[] = {"", "Gesture"};
        KH_MeleeHitFrontGesture[] = {"", "Gesture"};
        KH_MeleeHitBackGesture[] = {"", "Gesture"};
    };
    class States;
};
class CfgMovesMaleSdr: CfgMovesBasic
{
    class Actions: Actions
    {
        class CivilStandActions;
        class KH_MeleeTertiaryLightSwordActions: CivilStandActions
        {
            access = 3;
            CanNotMove = "Hum_AbsStaCom_IdlLhtMel";
            Civil = "Hum_AbsStaCom_IdlLhtMel";
            Combat = "Hum_AbsStaCom_IdlLhtMel";
            default = "Hum_AbsStaCom_IdlLhtMel";
            EvasiveLeft = "Hum_AbsStaCom_RunFwdLwdLhtMel";
            EvasiveRight = "Hum_AbsStaCom_RunFwdRwdLhtMel";
            FastB = "Hum_AbsStaCom_JogBwdLhtMel";
            FastF = "Hum_AbsStaCom_RunFwdLhtMel";
            FastL = "Hum_AbsStaCom_JogLwdLhtMel";
            FastLB = "Hum_AbsStaCom_JogBwdLwdLhtMel";
            FastLF = "Hum_AbsStaCom_RunFwdLwdLhtMel";
            FastR = "Hum_AbsStaCom_JogRwdLhtMel";
            FastRB = "Hum_AbsStaCom_JogBwdRwdLhtMel";
            FastRF = "Hum_AbsStaCom_RunFwdRwdLhtMel";
            FireNotPossible = "Hum_AbsStaCom_IdlLhtMel";
            KH_MeleeDodge = "Hum_AbsStaCom_DgeBwdLhtMel";
            KH_MeleeBlockGesture[] = {"Hum_AddStaAny_BloLhtMel", "Gesture"};
            KH_MeleeBlockIn = "Hum_AbsStaCom_BloLhtMel";
            KH_MeleeBlockOut = "Hum_AbsStaCom_IdlLhtMel";
            KH_MeleeBlockSuccess = "Hum_AbsStaCom_BloSucLhtMel";
            KH_MeleeBlockSuccessGesture[] = {"Hum_AddStaAny_BloSucLhtMel", "Gesture"};
            KH_MeleeBlockFailure = "Hum_AbsStaCom_StgHvyBwdLhtMel";
            KH_MeleeInLightSword = "Hum_AbsStaCom_IdlLhtMel";
            KH_MeleeKick = "Hum_AbsStaCom_KckLhtMel";
            KH_Attack01 = "Hum_AbsStaCom_AttLhtMel01A";
            KH_Attack02 = "Hum_AbsStaCom_AttLhtMel02A";
            KH_MeleeParry = "Hum_AbsStaCom_ParLhtMel";
            KH_MeleeStop = "Hum_AbsStaCom_IdlLhtMel";
            KH_MeleeRecoil = "Hum_AbsStaCom_RclRhtLhtMel";
            KH_MeleeTackle = "Hum_AbsStaCom_TklLhtMel";
            KH_MeleeHitLeftGesture[] = {"Hum_AddAnyAny_HitLftLhtMel", "Gesture"};
            KH_MeleeHitRightGesture[] = {"Hum_AddAnyAny_HitRhtLhtMel", "Gesture"};
            KH_MeleeHitFrontGesture[] = {"Hum_AddAnyAny_HitFroLhtMel", "Gesture"};
            KH_MeleeHitBackGesture[] = {"Hum_AddAnyAny_HitBckLhtMel", "Gesture"};
            KH_StaggerLightForward = "Hum_AbsStaCom_StgLhtFwdLhtMel";
            KH_StaggerLightBackward = "Hum_AbsStaCom_StgLhtBwdLhtMel";
            KH_StaggerLightLeftward = "Hum_AbsStaCom_StgLhtLwdLhtMel";
            KH_StaggerLightRightward = "Hum_AbsStaCom_StgLhtRwdLhtMel";
            KH_StaggerHeavyForward = "Hum_AbsStaCom_StgHvyFwdLhtMel";
            KH_StaggerHeavyBackward = "Hum_AbsStaCom_StgHvyBwdLhtMel";
            KH_StaggerHeavyLeftward = "Hum_AbsStaCom_StgHvyLwdLhtMel";
            KH_StaggerHeavyRightward = "Hum_AbsStaCom_StgHvyRwdLhtMel";
            leanLRot = 0.5;
            leanLShift = 0;
            leanRRot = 0.5;
            leanRShift = 0;
            limitFast = 4;
            LimpB = "Hum_AbsStaCom_WlkBwdLhtMel";
            LimpF = "Hum_AbsStaCom_WlkFwdLhtMel";
            LimpL = "Hum_AbsStaCom_WlkLwdLhtMel";
            LimpLB = "Hum_AbsStaCom_WlkBwdLwdLhtMel";
            LimpLF = "Hum_AbsStaCom_WlkFwdLwdLhtMel";
            LimpR = "Hum_AbsStaCom_WlkRwdLhtMel";
            LimpRB = "Hum_AbsStaCom_WlkBwdRwdLhtMel";
            LimpRF = "Hum_AbsStaCom_WlkFwdRwdLhtMel";
            obstructed = "Hum_AbsStaCom_IdlLhtMel";
            PlayerSlowB = "Hum_AbsStaCom_TroBwdLhtMel";
            PlayerSlowF = "Hum_AbsStaCom_TroFwdLhtMel";
            PlayerSlowL = "Hum_AbsStaCom_TroLwdLhtMel";
            PlayerSlowLB = "Hum_AbsStaCom_TroBwdLwdLhtMel";
            PlayerSlowLF = "Hum_AbsStaCom_TroFwdLwdLhtMel";
            PlayerSlowR = "Hum_AbsStaCom_TroRwdLhtMel";
            PlayerSlowRB = "Hum_AbsStaCom_TroBwdRwdLhtMel";
            PlayerSlowRF = "Hum_AbsStaCom_TroFwdRwdLhtMel";
            PlayerStand = "Hum_AbsStaCom_IdlLhtMel";
            PlayerTactB = "Hum_AbsStaCom_JogBwdLhtMel";
            PlayerTactF = "Hum_AbsStaCom_JogFwdLhtMel";
            PlayerTactL = "Hum_AbsStaCom_JogLwdLhtMel";
            PlayerTactLB = "Hum_AbsStaCom_JogBwdLwdLhtMel";
            PlayerTactLF = "Hum_AbsStaCom_JogFwdLwdLhtMel";
            PlayerTactR = "Hum_AbsStaCom_JogRwdLhtMel";
            PlayerTactRB = "Hum_AbsStaCom_JogBwdRwdLhtMel";
            PlayerTactRF = "Hum_AbsStaCom_JogFwdRwdLhtMel";
            PlayerWalkB = "Hum_AbsStaCom_WlkBwdLhtMel";
            PlayerWalkF = "Hum_AbsStaCom_WlkFwdLhtMel";
            PlayerWalkL = "Hum_AbsStaCom_WlkLwdLhtMel";
            PlayerWalkLB = "Hum_AbsStaCom_WlkBwdLwdLhtMel";
            PlayerWalkLF = "Hum_AbsStaCom_WlkFwdLwdLhtMel";
            PlayerWalkR = "Hum_AbsStaCom_WlkRwdLhtMel";
            PlayerWalkRB = "Hum_AbsStaCom_WlkBwdRwdLhtMel";
            PlayerWalkRF = "Hum_AbsStaCom_WlkFwdRwdLhtMel";
            Relax = "Hum_AbsStaCom_IdlLhtMel";
            SecondaryWeapon = "Hum_AbsStaCom_IdlLhtMel";
            SlowB = "Hum_AbsStaCom_TroBwdLhtMel";
            SlowF = "Hum_AbsStaCom_TroFwdLhtMel";
            SlowL = "Hum_AbsStaCom_TroLwdLhtMel";
            SlowLB = "Hum_AbsStaCom_TroBwdLwdLhtMel";
            SlowLF = "Hum_AbsStaCom_TroFwdLwdLhtMel";
            SlowR = "Hum_AbsStaCom_TroRwdLhtMel";
            SlowRB = "Hum_AbsStaCom_TroBwdRwdLhtMel";
            SlowRF = "Hum_AbsStaCom_TroFwdRwdLhtMel";
            Stand = "Hum_AbsStaCom_IdlLhtMel";
            stop = "Hum_AbsStaCom_IdlLhtMel";
            stopRelaxed = "Hum_AbsStaCom_IdlLhtMel";
            TactB = "Hum_AbsStaCom_JogBwdLhtMel";
            TactF = "Hum_AbsStaCom_JogFwdLhtMel";
            TactL = "Hum_AbsStaCom_JogLwdLhtMel";
            TactLB = "Hum_AbsStaCom_JogBwdLwdLhtMel";
            TactLF = "Hum_AbsStaCom_JogFwdLwdLhtMel";
            TactR = "Hum_AbsStaCom_JogRwdLhtMel";
            TactRB = "Hum_AbsStaCom_JogBwdRwdLhtMel";
            TactRF = "Hum_AbsStaCom_JogFwdRwdLhtMel";
            turnL = "Hum_AbsStaCom_TrnLftLhtMel";
            turnLRelaxed = "Hum_AbsStaCom_TrnLftLhtMel";
            turnR = "Hum_AbsStaCom_TrnRhtLhtMel";
            turnRRelaxed = "Hum_AbsStaCom_TrnRhtLhtMel";
            turnSpeed = 8;
            WalkB = "Hum_AbsStaCom_WlkBwdLhtMel";
            WalkF = "Hum_AbsStaCom_WlkFwdLhtMel";
            WalkL = "Hum_AbsStaCom_WlkLwdLhtMel";
            WalkLB = "Hum_AbsStaCom_WlkBwdLwdLhtMel";
            WalkLF = "Hum_AbsStaCom_WlkFwdLwdLhtMel";
            WalkR = "Hum_AbsStaCom_WlkRwdLhtMel";
            WalkRB = "Hum_AbsStaCom_WlkBwdRwdLhtMel";
            WalkRF = "Hum_AbsStaCom_WlkFwdRwdLhtMel";
            weaponOn = "Hum_AbsStaCom_IdlLhtMel";
            weaponOff = "Hum_AbsStaCom_IdlLhtMel";
        };
        class KH_MeleeTertiaryLightSwordBlock: KH_MeleeTertiaryLightSwordActions
        {
            CanNotMove = "Hum_AbsStaCom_BloLhtMel";
            Combat = "Hum_AbsStaCom_BloLhtMel";
            Civil = "Hum_AbsStaCom_BloLhtMel";
            default = "Hum_AbsStaCom_BloLhtMel";
            FireNotPossible = "Hum_AbsStaCom_BloLhtMel";
            obstructed = "Hum_AbsStaCom_BloLhtMel";
            Relax = "Hum_AbsStaCom_BloLhtMel";
            SecondaryWeapon = "Hum_AbsStaCom_BloLhtMel";
            stop = "Hum_AbsStaCom_BloLhtMel";
            stopRelaxed = "Hum_AbsStaCom_BloLhtMel";
            turnL = "Hum_AbsStaCom_BloLhtMel";
            turnLRelaxed = "Hum_AbsStaCom_BloLhtMel";
            turnR = "Hum_AbsStaCom_BloLhtMel";
            turnRRelaxed = "Hum_AbsStaCom_BloLhtMel";
            weaponOff = "Hum_AbsStaCom_BloLhtMel";
            weaponOn = "Hum_AbsStaCom_BloLhtMel";
        };
        class KH_MeleeTertiaryLightSwordForward: KH_MeleeTertiaryLightSwordActions
        {
            KH_MeleeDodge = "Hum_AbsStaCom_DgeFwdLhtMel";
        };
        class KH_MeleeTertiaryLightSwordBackward: KH_MeleeTertiaryLightSwordActions
        {
            KH_MeleeDodge = "Hum_AbsStaCom_DgeBwdLhtMel";
        };
        class KH_MeleeTertiaryLightSwordLeftward: KH_MeleeTertiaryLightSwordActions
        {
            KH_MeleeDodge = "Hum_AbsStaCom_DgeLwdLhtMel";
        };
        class KH_MeleeTertiaryLightSwordRightward: KH_MeleeTertiaryLightSwordActions
        {
            KH_MeleeDodge = "Hum_AbsStaCom_DgeRwdLhtMel";
        };
        class KH_MeleeTertiaryMeleeLightSwordAttack01A: KH_MeleeTertiaryLightSwordActions
        {
            KH_Attack01 = "Hum_AbsStaCom_AttLhtMel01B";
            KH_Attack02 = "Hum_AbsStaCom_AttLhtMel02B";
        };
        class KH_MeleeTertiaryMeleeLightSwordAttack01B: KH_MeleeTertiaryLightSwordActions
        {
            KH_Attack01 = "Hum_AbsStaCom_AttLhtMel01C";
            KH_Attack02 = "Hum_AbsStaCom_AttLhtMel02C";
        };
        class KH_MeleeTertiaryMeleeLightSwordAttack01C: KH_MeleeTertiaryLightSwordActions
        {
            KH_Attack01 = "Hum_AbsStaCom_IdlLhtMel";
            KH_Attack02 = "Hum_AbsStaCom_IdlLhtMel";
        };
        class KH_MeleeTertiaryMeleeLightSwordAttack02A: KH_MeleeTertiaryLightSwordActions
        {
            KH_Attack01 = "Hum_AbsStaCom_AttLhtMel01B";
            KH_Attack02 = "Hum_AbsStaCom_AttLhtMel02B";
        };
        class KH_MeleeTertiaryMeleeLightSwordAttack02B: KH_MeleeTertiaryLightSwordActions
        {
            KH_Attack01 = "Hum_AbsStaCom_AttLhtMel01C";
            KH_Attack02 = "Hum_AbsStaCom_AttLhtMel02C";
        };
        class KH_MeleeTertiaryMeleeLightSwordAttack02C: KH_MeleeTertiaryLightSwordActions
        {
            KH_Attack01 = "Hum_AbsStaCom_IdlLhtMel";
            KH_Attack02 = "Hum_AbsStaCom_IdlLhtMel";
        };
    };
    class StandBase;
    class States: States
    {
        class KH_MeleeBase
        {
            access = 3;
            actions = "KH_MeleeTertiaryLightSwordActions";
            adjstance = "m";
            aiming = "aimingDefault";
            aimingBody = "aimingUpDefault";
            aimPrecision = 0;
            boundingSphere = 1;
            camShakeFire = 0;
            canBlendStep = 1;
            canPullTrigger = 0;
            canReload = 0;
            collisionShape = "A3\anims_f\data\geom\sdr\Perc_Wrfl_Low.p3d";
            collisionShapeSafe = "";
            connectAs = "";
            connectFrom[] = {};
            ConnectTo[] = {};
            disableWeapons = 1;
            disableWeaponsLong = 1;
            duty = 0;
            enableAutoActions = 0;
            enableBinocular = 0;
            enableDirectControl = 1;
            enableMissile = 0;
            enableOptics = 0;
            equivalentTo = "";
            file = "x\kh\addons\main\animations\human\Hum_AbsStaSaf_IdlLhtMel.rtm";
            forceAim = 0;
            hasCollShapeSafe = 0;
            head = "headDefault";
            headBobMode = 0;
            headBobStrength = 0;
            idle = "idleDefault";
            ignoreMinPlayTime[] = {
                "Unconscious",
                "Hum_AbsStaCom_StgHvyBwdLhtMel",
                "Hum_AbsStaCom_StgHvyFwdLhtMel",
                "Hum_AbsStaCom_StgHvyLwdLhtMel",
                "Hum_AbsStaCom_StgHvyRwdLhtMel",
                "Hum_AbsStaCom_StgLhtBwdLhtMel",
                "Hum_AbsStaCom_StgLhtFwdLhtMel",
                "Hum_AbsStaCom_StgLhtLwdLhtMel",
                "Hum_AbsStaCom_StgLhtRwdLhtMel",
                "Hum_AbsStaCom_RclRhtLhtMel"
            };
            interpolateFrom[] = {};
            InterpolateTo[] = {"Unconscious", 0.02};
            interpolateWith[] = {};
            interpolationRestart = 0;
            interpolationSpeed = 4;
            kh_melee = 1;
            leaning = "leaningDefault";
            leaningFactorBeg = 0;
            leaningFactorEnd = 0;
            leaningFactorZeroPoint = -1;
            leftHandIKBeg = 0;
            leftHandIKCurve[] = {0};
            leftHandIKEnd = 0;
            legs = "legsDefault";
            limitGunMovement = 0.25;
            looped = 1;
            mask = "weaponSwitching";
            minPlayTime = 0;
            onLadder = 0;
            onLandBeg = 0;
            onLandEnd = 0;
            preload = 1;
            ragdoll = 1;
            relSpeedMax = 1;
            relSpeedMin = 1;
            rightHandIKBeg = 1;
            rightHandIKCurve[] = {1};
            rightHandIKEnd = 1;
            showHandGun = 0;
            showItemInHand = 0;
            showItemInRightHand = 0;
            showWeaponAim = 0;
            soundEdge[] = {0.5, 1};
            soundEnabled = 1;
            soundOverride = "";
            speed = 1e+10;
            stamina = 1;
            static = 1;
            terminal = 0;
            useIdles = 1;
            variantAfter[] = {1, 1, 1};
            variantsAI[] = {};
            variantsPlayer[] = {};
            visiblesize = 0.6;
            Walkcycles = 1;
            weaponIK = 4;
            weaponLowered = 0;
            weaponObstructed = 0;
        };
        class KH_MeleeBespokeBase: KH_MeleeBase
        {
            canBlendStep = 0;
            looped = 0;
            interpolationRestart = 1;
            interpolationSpeed = 8;
        };
        class Hum_AbsStaCom_IdlLhtMel: KH_MeleeBase
        {
            canBlendStep = 0;
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_IdlLhtMel.rtm";
            speed = 1e+10;
            interpolationSpeed = 6;
            connectTo[] = {
                "Hum_AbsStaCom_AttLhtMel01A", 0.1,
                "Hum_AbsStaCom_AttLhtMel02A", 0.1,
                "Hum_AbsStaCom_DgeBwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeFwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeLwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeRwdLhtMel", 0.1,
                "Hum_AbsStaCom_ParLhtMel", 0.1,
                "Hum_AbsStaCom_KckLhtMel", 0.1,
                "Hum_AbsStaCom_TklLhtMel", 0.1,
                "Hum_AbsStaCom_AttLhtMelCbo01", 0.1,
                "Hum_AbsStaCom_AttLhtMelCbo02", 0.1,
                "Hum_AbsStaCom_IdlLhtMel_Tra_AbsStaSaf_IdlLhtMel", 0.1
            };
            interpolateTo[] += {
                "Hum_AbsStaCom_BloLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkRwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TrnLftLhtMel", 0.1,
                "Hum_AbsStaCom_TrnRhtLhtMel", 0.1,
                "Hum_AbsStaCom_StgHvyBwdLhtMel", 0.1,
                "Hum_AbsStaCom_StgHvyFwdLhtMel", 0.1,
                "Hum_AbsStaCom_StgHvyLwdLhtMel", 0.1,
                "Hum_AbsStaCom_StgHvyRwdLhtMel", 0.1,
                "Hum_AbsStaCom_StgLhtBwdLhtMel", 0.1,
                "Hum_AbsStaCom_StgLhtFwdLhtMel", 0.1,
                "Hum_AbsStaCom_StgLhtLwdLhtMel", 0.1,
                "Hum_AbsStaCom_StgLhtRwdLhtMel", 0.1
            };
        };
        class Hum_AbsStaCom_AttLhtMel01A: KH_MeleeBespokeBase
        {
            actions = "KH_MeleeTertiaryMeleeLightSwordAttack01A";
            speed = 1.25;
            minPlayTime = 1;
            kh_meleeHitTiming[] = {
                {0.7667, "launcher", {0.571089, 0.327445, 1.878061}, "Attack01", 0},
                {0.8000, "launcher", {0.518998, 0.651819, 1.631181}, "Attack01", 0},
                {0.8333, "launcher", {0.328988, 0.874207, 1.297552}, "Attack01", 0},
                {0.8667, "launcher", {-0.017715, 0.928013, 1.079567}, "Attack01", 0},
                {0.9000, "launcher", {-0.384748, 0.787088, 1.002090}, "Attack01", 0}
            };
            kh_meleeSoundTiming[] = {
                {0.7667, "launcher", "Attack01", "swing"}
            };
            soundOverride = "Walk";
            soundEdge[] = {0.85};
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_AttLhtMel01A.rtm";
            connectTo[] = {
                "Hum_AbsStaCom_AttLhtMel01B", 0.2,
                "Hum_AbsStaCom_AttLhtMel01A_Tra_AbsStaCom_IdlLhtMel", 0.1
            };
            interpolateTo[] = {
                "Hum_AbsStaCom_AttLhtMel02B", 0.15,
                "Hum_AbsStaCom_RclRhtLhtMel", 0.2
            };
        };
        class Hum_AbsStaCom_AttLhtMel01B: KH_MeleeBespokeBase
        {
            actions = "KH_MeleeTertiaryMeleeLightSwordAttack01B";
            speed = 1.7857125;
            minPlayTime = 1;
            kh_meleeHitTiming[] = {
                {0.6190, "launcher", {-0.184743, 0.285979, 1.863449}, "Attack01", 0},
                {0.6667, "launcher", {-0.038018, 0.555262, 1.668209}, "Attack01", 0},
                {0.7143, "launcher", {0.285073, 0.675379, 1.310326}, "Attack01", 0},
                {0.7619, "launcher", {0.666743, 0.423554, 0.925992}, "Attack01", 0}
            };
            kh_meleeSoundTiming[] = {
                {0.6190, "launcher", "Attack01", "swing"}
            };
            soundOverride = "Walk";
            soundEdge[] = {0.85};
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_AttLhtMel01B.rtm";
            connectTo[] = {
                "Hum_AbsStaCom_AttLhtMel01C", 0.2,
                "Hum_AbsStaCom_AttLhtMel01B_Tra_AbsStaCom_IdlLhtMel", 0.1
            };
            interpolateTo[] = {
                "Hum_AbsStaCom_AttLhtMel02C", 0.15,
                "Hum_AbsStaCom_RclRhtLhtMel", 0.2
            };
        };
        class Hum_AbsStaCom_AttLhtMel01C: KH_MeleeBespokeBase
        {
            actions = "KH_MeleeTertiaryMeleeLightSwordAttack01C";
            speed = 0.5952375;
            minPlayTime = 0.75;
            soundOverride = "Walk";
            soundEdge[] = {0.3, 0.7, 0.9};
            kh_meleeHitTiming[] = {
                {0.2222, "launcher", {0.767645, 0.414563, 1.769273}, "Attack01", 0},
                {0.2381, "launcher", {0.643957, 0.848914, 1.438889}, "Attack01", 0},
                {0.2540, "launcher", {0.261746, 1.072362, 1.209595}, "Attack01", 0},
                {0.2698, "launcher", {-0.181196, 1.009021, 0.976096}, "Attack01", 0}
            };
            kh_meleeSoundTiming[] = {
                {0.2222, "launcher", "Attack01", "swing"}
            };
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_AttLhtMel01C.rtm";
            connectTo[] = {
                "Hum_AbsStaCom_IdlLhtMel", 0.1
            };
            interpolateTo[] = {
                "Hum_AbsStaCom_AttLhtMel01A", 0.15,
                "Hum_AbsStaCom_AttLhtMel02A", 0.15,
                "Hum_AbsStaCom_DgeBwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeFwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeLwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeRwdLhtMel", 0.1,
                "Hum_AbsStaCom_RclRhtLhtMel", 0.2,
                "Hum_AbsStaCom_JogBwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkRwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TrnLftLhtMel", 0.1,
                "Hum_AbsStaCom_TrnRhtLhtMel", 0.1
            };
        };
        class Hum_AbsStaCom_AttLhtMel02A: KH_MeleeBespokeBase
        {
            actions = "KH_MeleeTertiaryMeleeLightSwordAttack02A";
            speed = 1.10294125;
            minPlayTime = 1;
            kh_meleeHitTiming[] = {
                {0.7941, "launcher", {0.228647, 0.809746, 0.936372}, "Attack02", 0},
                {0.8235, "launcher", {0.164290, 0.909469, 1.052379}, "Attack02", 0},
                {0.8529, "launcher", {0.132857, 0.943481, 1.135512}, "Attack02", 0},
                {0.8824, "launcher", {0.136187, 0.952689, 1.144455}, "Attack02", 0},
                {0.9118, "launcher", {0.150902, 0.944878, 1.104812}, "Attack02", 0}
            };
            kh_meleeSoundTiming[] = {
                {0.7941, "launcher", "Attack02", "thrust"}
            };
            soundOverride = "Walk";
            soundEdge[] = {0.2, 0.8};
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_AttLhtMel02A.rtm";
            connectTo[] = {
                "Hum_AbsStaCom_AttLhtMel02A_Tra_AbsStaCom_IdlLhtMel", 0.1,
                "Hum_AbsStaCom_AttLhtMel02B", 0.2
            };
            interpolateTo[] = {
                "Hum_AbsStaCom_AttLhtMel01B", 0.15,
                "Hum_AbsStaCom_RclRhtLhtMel", 0.2
            };
        };
        class Hum_AbsStaCom_AttLhtMel02B: KH_MeleeBespokeBase
        {
            actions = "KH_MeleeTertiaryMeleeLightSwordAttack02B";
            speed = 1.171875;
            minPlayTime = 1;
            kh_meleeHitTiming[] = {
                {0.5938, "launcher", {0.700077, 0.387612, 1.229074}, "Attack02", 0},
                {0.6250, "launcher", {0.507225, 0.577151, 1.121093}, "Attack02", 0},
                {0.6562, "launcher", {0.302800, 0.681197, 1.051666}, "Attack02", 0},
                {0.6875, "launcher", {0.145991, 0.732901, 1.016608}, "Attack02", 0}
            };
            kh_meleeSoundTiming[] = {
                {0.5938, "launcher", "Attack02", "thrust"}
            };
            soundOverride = "Walk";
            soundEdge[] = {0.68};
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_AttLhtMel02B.rtm";
            connectTo[] = {
                "Hum_AbsStaCom_AttLhtMel02C", 0.2,
                "Hum_AbsStaCom_AttLhtMel02B_Tra_AbsStaCom_IdlLhtMel", 0.1
            };
            interpolateTo[] = {
                "Hum_AbsStaCom_AttLhtMel01C", 0.15,
                "Hum_AbsStaCom_RclRhtLhtMel", 0.2
            };
        };
        class Hum_AbsStaCom_AttLhtMel02C: KH_MeleeBespokeBase
        {
            actions = "KH_MeleeTertiaryMeleeLightSwordAttack02C";
            speed = 0.56818125;
            minPlayTime = 0.6;
            kh_meleeHitTiming[] = {
                {0.1515, "launcher", {0.402936, 0.870261, 1.535653}, "Attack02", 0},
                {0.1667, "launcher", {0.113386, 1.034553, 1.303826}, "Attack02", 0},
                {0.1818, "launcher", {-0.113235, 1.052763, 1.031356}, "Attack02", 0},
                {0.1970, "launcher", {-0.311981, 0.961651, 0.881793}, "Attack02", 0}
            };
            kh_meleeSoundTiming[] = {
                {0.1515, "launcher", "Attack02", "thrust"}
            };
            soundOverride = "Walk";
            soundEdge[] = {0.21, 0.5, 0.8};
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_AttLhtMel02C.rtm";
            connectTo[] = {
                "Hum_AbsStaCom_IdlLhtMel", 0.1
            };
            interpolateTo[] = {
                "Hum_AbsStaCom_AttLhtMel01A", 0.15,
                "Hum_AbsStaCom_AttLhtMel02A", 0.15,
                "Hum_AbsStaCom_DgeBwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeFwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeLwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeRwdLhtMel", 0.1,
                "Hum_AbsStaCom_RclRhtLhtMel", 0.2,
                "Hum_AbsStaCom_JogBwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkRwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TrnLftLhtMel", 0.1,
                "Hum_AbsStaCom_TrnRhtLhtMel", 0.1
            };
        };
        class Hum_AbsStaCom_AttLhtMelCbo01: KH_MeleeBespokeBase
        {
            speed = 0.267857;
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_AttLhtMelCbo01.rtm";
            connectTo[] = {
                "Hum_AbsStaCom_IdlLhtMel", 0.1
            };
            interpolateTo[] = {};
        };
        class Hum_AbsStaCom_AttLhtMelCbo02: KH_MeleeBespokeBase
        {
            speed = 0.230769;
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_AttLhtMelCbo02.rtm";
            connectTo[] = {
                "Hum_AbsStaCom_IdlLhtMel", 0.1
            };
            interpolateTo[] = {};
        };
        class Hum_AbsStaCom_BloLhtMel: KH_MeleeBase
        {
            actions = "KH_MeleeTertiaryLightSwordBlock";
            speed = 1e+10;
            kh_meleeBlockTiming[] = {
                {0, 1, "Block01"}
            };
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_BloLhtMel.rtm";
            connectTo[] = {};
            interpolateTo[] = {
                "Hum_AbsStaCom_IdlLhtMel", 0.1,
                "Hum_AbsStaCom_BloSucLhtMel", 0.2
            };
        };
        class Hum_AbsStaCom_BloSucLhtMel: KH_MeleeBespokeBase
        {
            speed = 1.57895;
            minPlayTime = 1;
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_BloSucLhtMel.rtm";
            connectTo[] = {
                "Hum_AbsStaCom_IdlLhtMel", 0.1
            };
            interpolateTo[] = {};
        };
        class Hum_AbsStaCom_DgeBwdLhtMel: KH_MeleeBespokeBase
        {
            actions = "KH_MeleeTertiaryLightSwordBackward";
            speed = 1.03448;
            minPlayTime = 0.6;
            kh_meleeDodgeTiming[] = {
                {0.2, 0.8, 180}
            };
            soundOverride = "run";
            soundEdge[] = {0.5, 0.75};
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_DgeBwdLhtMel.rtm";
            connectTo[] = {
                "Hum_AbsStaCom_IdlLhtMel", 0.1
            };
            interpolateTo[] = {
                "Hum_AbsStaCom_DgeFwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeLwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeRwdLhtMel", 0.1,
                "Hum_AbsStaCom_AttLhtMel01A", 0.1,
                "Hum_AbsStaCom_AttLhtMel02A", 0.1,
                "Hum_AbsStaCom_JogBwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkRwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TrnLftLhtMel", 0.1,
                "Hum_AbsStaCom_TrnRhtLhtMel", 0.1,
                "Hum_AbsStaCom_TklLhtMel", 0.1
            };
        };
        class Hum_AbsStaCom_DgeFwdLhtMel: KH_MeleeBespokeBase
        {
            actions = "KH_MeleeTertiaryLightSwordForward";
            speed = 1.15385;
            minPlayTime = 0.6;
            kh_meleeDodgeTiming[] = {
                {0.2, 0.8, 0}
            };
            soundOverride = "run";
            soundEdge[] = {0.5, 0.75};
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_DgeFwdLhtMel.rtm";
            connectTo[] = {
                "Hum_AbsStaCom_IdlLhtMel", 0.1
            };
            interpolateTo[] = {
                "Hum_AbsStaCom_DgeBwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeLwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeRwdLhtMel", 0.1,
                "Hum_AbsStaCom_AttLhtMel01A", 0.1,
                "Hum_AbsStaCom_AttLhtMel02A", 0.1,
                "Hum_AbsStaCom_JogBwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkRwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TrnLftLhtMel", 0.1,
                "Hum_AbsStaCom_TrnRhtLhtMel", 0.1,
                "Hum_AbsStaCom_TklLhtMel", 0.1
            };
        };
        class Hum_AbsStaCom_DgeLwdLhtMel: KH_MeleeBespokeBase
        {
            actions = "KH_MeleeTertiaryLightSwordLeftward";
            speed = 1.11111;
            minPlayTime = 0.6;
            kh_meleeDodgeTiming[] = {
                {0.2, 0.8, 270}
            };
            soundOverride = "run";
            soundEdge[] = {0.65, 0.85};
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_DgeLwdLhtMel.rtm";
            connectTo[] = {
                "Hum_AbsStaCom_IdlLhtMel", 0.1
            };
            interpolateTo[] = {
                "Hum_AbsStaCom_DgeBwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeFwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeRwdLhtMel", 0.1,
                "Hum_AbsStaCom_AttLhtMel01A", 0.1,
                "Hum_AbsStaCom_AttLhtMel02A", 0.1,
                "Hum_AbsStaCom_JogBwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkRwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TrnLftLhtMel", 0.1,
                "Hum_AbsStaCom_TrnRhtLhtMel", 0.1,
                "Hum_AbsStaCom_TklLhtMel", 0.1
            };
        };
        class Hum_AbsStaCom_DgeRwdLhtMel: KH_MeleeBespokeBase
        {
            actions = "KH_MeleeTertiaryLightSwordRightward";
            speed = 1.11111;
            minPlayTime = 0.6;
            kh_meleeDodgeTiming[] = {
                {0.2, 0.8, 90}
            };
            soundOverride = "run";
            soundEdge[] = {0.65, 0.8};
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_DgeRwdLhtMel.rtm";
            connectTo[] = {
                "Hum_AbsStaCom_IdlLhtMel", 0.1
            };
            interpolateTo[] = {
                "Hum_AbsStaCom_DgeBwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeFwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeLwdLhtMel", 0.1,
                "Hum_AbsStaCom_AttLhtMel01A", 0.1,
                "Hum_AbsStaCom_AttLhtMel02A", 0.1,
                "Hum_AbsStaCom_JogBwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkRwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TrnLftLhtMel", 0.1,
                "Hum_AbsStaCom_TrnRhtLhtMel", 0.1,
                "Hum_AbsStaCom_TklLhtMel", 0.1
            };
        };
        class Hum_AbsStaCom_JogBwdLhtMel: KH_MeleeBase
        {
            actions = "KH_MeleeTertiaryLightSwordBackward";
            speed = 0.789474;
            Walkcycles = 2;
            soundOverride = "run";
            soundEdge[] = {0.25, 0.5, 0.75, 1};
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_JogBwdLhtMel.rtm";
            connectTo[] = {};
            interpolateTo[] = {
                "Hum_AbsStaCom_JogBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkRwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroRwdLhtMel", 0.1,
                "Hum_AbsStaCom_IdlLhtMel", 0.1,
                "Hum_AbsStaCom_TrnLftLhtMel", 0.1,
                "Hum_AbsStaCom_TrnRhtLhtMel", 0.1,
                "Hum_AbsStaCom_AttLhtMel01A", 0.1,
                "Hum_AbsStaCom_AttLhtMel02A", 0.1,
                "Hum_AbsStaCom_DgeBwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeFwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeLwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TklLhtMel", 0.1
            };
        };
        class Hum_AbsStaCom_JogBwdLwdLhtMel: KH_MeleeBase
        {
            actions = "KH_MeleeTertiaryLightSwordBackward";
            speed = 0.789474;
            Walkcycles = 2;
            soundOverride = "run";
            soundEdge[] = {0.25, 0.5, 0.75, 1};
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_JogBwdLwdLhtMel.rtm";
            connectTo[] = {};
            interpolateTo[] = {
                "Hum_AbsStaCom_JogBwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkRwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroRwdLhtMel", 0.1,
                "Hum_AbsStaCom_IdlLhtMel", 0.1,
                "Hum_AbsStaCom_TrnLftLhtMel", 0.1,
                "Hum_AbsStaCom_TrnRhtLhtMel", 0.1,
                "Hum_AbsStaCom_AttLhtMel01A", 0.1,
                "Hum_AbsStaCom_AttLhtMel02A", 0.1,
                "Hum_AbsStaCom_DgeBwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeFwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeLwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TklLhtMel", 0.1
            };
        };
        class Hum_AbsStaCom_JogBwdRwdLhtMel: KH_MeleeBase
        {
            actions = "KH_MeleeTertiaryLightSwordBackward";
            speed = 0.789474;
            Walkcycles = 2;
            soundOverride = "run";
            soundEdge[] = {0.25, 0.5, 0.75, 1};
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_JogBwdRwdLhtMel.rtm";
            connectTo[] = {};
            interpolateTo[] = {
                "Hum_AbsStaCom_JogBwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkRwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroRwdLhtMel", 0.1,
                "Hum_AbsStaCom_IdlLhtMel", 0.1,
                "Hum_AbsStaCom_TrnLftLhtMel", 0.1,
                "Hum_AbsStaCom_TrnRhtLhtMel", 0.1,
                "Hum_AbsStaCom_AttLhtMel01A", 0.1,
                "Hum_AbsStaCom_AttLhtMel02A", 0.1,
                "Hum_AbsStaCom_DgeBwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeFwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeLwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TklLhtMel", 0.1
            };
        };
        class Hum_AbsStaCom_JogFwdLhtMel: KH_MeleeBase
        {
            actions = "KH_MeleeTertiaryLightSwordForward";
            speed = 0.769231;
            Walkcycles = 2;
            soundOverride = "run";
            soundEdge[] = {0.25, 0.5, 0.75, 1};
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_JogFwdLhtMel.rtm";
            connectTo[] = {};
            interpolateTo[] = {
                "Hum_AbsStaCom_JogBwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkRwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroRwdLhtMel", 0.1,
                "Hum_AbsStaCom_IdlLhtMel", 0.1,
                "Hum_AbsStaCom_TrnLftLhtMel", 0.1,
                "Hum_AbsStaCom_TrnRhtLhtMel", 0.1,
                "Hum_AbsStaCom_AttLhtMel01A", 0.1,
                "Hum_AbsStaCom_AttLhtMel02A", 0.1,
                "Hum_AbsStaCom_DgeBwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeFwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeLwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TklLhtMel", 0.1
            };
        };
        class Hum_AbsStaCom_JogFwdLwdLhtMel: KH_MeleeBase
        {
            actions = "KH_MeleeTertiaryLightSwordForward";
            speed = 0.789474;
            Walkcycles = 2;
            soundOverride = "run";
            soundEdge[] = {0.25, 0.5, 0.75, 1};
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_JogFwdLwdLhtMel.rtm";
            connectTo[] = {};
            interpolateTo[] = {
                "Hum_AbsStaCom_JogBwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkRwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroRwdLhtMel", 0.1,
                "Hum_AbsStaCom_IdlLhtMel", 0.1,
                "Hum_AbsStaCom_TrnLftLhtMel", 0.1,
                "Hum_AbsStaCom_TrnRhtLhtMel", 0.1,
                "Hum_AbsStaCom_AttLhtMel01A", 0.1,
                "Hum_AbsStaCom_AttLhtMel02A", 0.1,
                "Hum_AbsStaCom_DgeBwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeFwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeLwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TklLhtMel", 0.1
            };
        };
        class Hum_AbsStaCom_JogFwdRwdLhtMel: KH_MeleeBase
        {
            actions = "KH_MeleeTertiaryLightSwordForward";
            speed = 0.789474;
            Walkcycles = 2;
            soundOverride = "run";
            soundEdge[] = {0.25, 0.5, 0.75, 1};
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_JogFwdRwdLhtMel.rtm";
            connectTo[] = {};
            interpolateTo[] = {
                "Hum_AbsStaCom_JogBwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkRwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroRwdLhtMel", 0.1,
                "Hum_AbsStaCom_IdlLhtMel", 0.1,
                "Hum_AbsStaCom_TrnLftLhtMel", 0.1,
                "Hum_AbsStaCom_TrnRhtLhtMel", 0.1,
                "Hum_AbsStaCom_AttLhtMel01A", 0.1,
                "Hum_AbsStaCom_AttLhtMel02A", 0.1,
                "Hum_AbsStaCom_DgeBwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeFwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeLwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TklLhtMel", 0.1
            };
        };
        class Hum_AbsStaCom_JogLwdLhtMel: KH_MeleeBase
        {
            actions = "KH_MeleeTertiaryLightSwordLeftward";
            speed = 0.75;
            Walkcycles = 2;
            soundOverride = "run";
            soundEdge[] = {0.25, 0.5, 0.75, 1};
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_JogLwdLhtMel.rtm";
            connectTo[] = {};
            interpolateTo[] = {
                "Hum_AbsStaCom_JogBwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkRwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroRwdLhtMel", 0.1,
                "Hum_AbsStaCom_IdlLhtMel", 0.1,
                "Hum_AbsStaCom_TrnLftLhtMel", 0.1,
                "Hum_AbsStaCom_TrnRhtLhtMel", 0.1,
                "Hum_AbsStaCom_AttLhtMel01A", 0.1,
                "Hum_AbsStaCom_AttLhtMel02A", 0.1,
                "Hum_AbsStaCom_DgeBwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeFwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeLwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TklLhtMel", 0.1
            };
        };
        class Hum_AbsStaCom_JogRwdLhtMel: KH_MeleeBase
        {
            actions = "KH_MeleeTertiaryLightSwordRightward";
            speed = 0.833333;
            Walkcycles = 2;
            soundOverride = "run";
            soundEdge[] = {0.25, 0.5, 0.75, 1};
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_JogRwdLhtMel.rtm";
            connectTo[] = {};
            interpolateTo[] = {
                "Hum_AbsStaCom_JogBwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkRwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroRwdLhtMel", 0.1,
                "Hum_AbsStaCom_IdlLhtMel", 0.1,
                "Hum_AbsStaCom_TrnLftLhtMel", 0.1,
                "Hum_AbsStaCom_TrnRhtLhtMel", 0.1,
                "Hum_AbsStaCom_AttLhtMel01A", 0.1,
                "Hum_AbsStaCom_AttLhtMel02A", 0.1,
                "Hum_AbsStaCom_DgeBwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeFwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeLwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TklLhtMel", 0.1
            };
        };
        class Hum_AbsStaCom_KckLhtMel: KH_MeleeBespokeBase
        {
            speed = 0.605263;
            kh_meleeKickTiming[] = {
                {0.35087, "RightFoot", "Kick01"}
            };
            soundOverride = "Walk";
            soundEdge[] = {0.14, 0.73, 0.91};
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_KckLhtMel.rtm";
            minPlayTime = 0.8;
            connectTo[] = {
                "Hum_AbsStaCom_IdlLhtMel", 0.1
            };
            interpolateTo[] = {
                "Hum_AbsStaCom_JogBwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkRwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TrnLftLhtMel", 0.1,
                "Hum_AbsStaCom_TrnRhtLhtMel", 0.1,
                "Hum_AbsStaCom_TklLhtMel", 0.1
            };
        };
        class Hum_AbsStaCom_ParLhtMel: KH_MeleeBespokeBase
        {
            speed = 0.697674;
            kh_meleeParryTiming[] = {
                {0.1, 0.6, "Parry01"}
            };
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_ParLhtMel.rtm";
            minPlayTime = 0.5;
            connectTo[] = {
                "Hum_AbsStaCom_IdlLhtMel", 0.1
            };
            interpolateTo[] = {};
        };
        class Hum_AbsStaCom_RclRhtLhtMel: KH_MeleeBespokeBase
        {
            speed = 0.638298;
            minPlayTime = 0.9;
            soundOverride = "Walk";
            soundEdge[] = {0.21, 0.65, 0.9};
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_RclRhtLhtMel.rtm";
            connectTo[] = {};
            interpolateTo[] = {
                "Hum_AbsStaCom_IdlLhtMel", 0.1
            };
        };
        class Hum_AbsStaCom_RunFwdLhtMel: KH_MeleeBase
        {
            actions = "KH_MeleeTertiaryLightSwordForward";
            speed = 1.75;
            soundOverride = "sprint";
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_RunFwdLhtMel.rtm";
            connectTo[] = {};
            interpolateTo[] = {
                "Hum_AbsStaCom_RunFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkRwdLhtMel", 0.1,
                "Hum_AbsStaCom_IdlLhtMel", 0.1,
                "Hum_AbsStaCom_TrnLftLhtMel", 0.1,
                "Hum_AbsStaCom_TrnRhtLhtMel", 0.1,
                "Hum_AbsStaCom_AttLhtMel01A", 0.1,
                "Hum_AbsStaCom_AttLhtMel02A", 0.1,
                "Hum_AbsStaCom_DgeBwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeFwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeLwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TklLhtMel", 0.1
            };
        };
        class Hum_AbsStaCom_RunFwdLwdLhtMel: KH_MeleeBase
        {
            actions = "KH_MeleeTertiaryLightSwordForward";
            speed = 1.75;
            soundOverride = "sprint";
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_RunFwdLwdLhtMel.rtm";
            connectTo[] = {};
            interpolateTo[] = {
                "Hum_AbsStaCom_RunFwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkRwdLhtMel", 0.1,
                "Hum_AbsStaCom_IdlLhtMel", 0.1,
                "Hum_AbsStaCom_TrnLftLhtMel", 0.1,
                "Hum_AbsStaCom_TrnRhtLhtMel", 0.1,
                "Hum_AbsStaCom_AttLhtMel01A", 0.1,
                "Hum_AbsStaCom_AttLhtMel02A", 0.1,
                "Hum_AbsStaCom_DgeBwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeFwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeLwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TklLhtMel", 0.1
            };
        };
        class Hum_AbsStaCom_RunFwdRwdLhtMel: KH_MeleeBase
        {
            actions = "KH_MeleeTertiaryLightSwordForward";
            speed = 1.75;
            soundOverride = "sprint";
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_RunFwdRwdLhtMel.rtm";
            connectTo[] = {};
            interpolateTo[] = {
                "Hum_AbsStaCom_RunFwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkRwdLhtMel", 0.1,
                "Hum_AbsStaCom_IdlLhtMel", 0.1,
                "Hum_AbsStaCom_TrnLftLhtMel", 0.1,
                "Hum_AbsStaCom_TrnRhtLhtMel", 0.1,
                "Hum_AbsStaCom_AttLhtMel01A", 0.1,
                "Hum_AbsStaCom_AttLhtMel02A", 0.1,
                "Hum_AbsStaCom_DgeBwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeFwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeLwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TklLhtMel", 0.1
            };
        };
        class Hum_AbsStaCom_StgHvyBwdLhtMel: KH_MeleeBespokeBase
        {
            speed = 0.810811;
            minPlayTime = 1;
            soundOverride = "sprint";
            soundEdge[] = {0.62, 0.83};
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_StgHvyBwdLhtMel.rtm";
            forceAim = 1;
            connectTo[] = {
                "Hum_AbsStaCom_StgHvyBwdLhtMel_Tra_AbsStaCom_IdlLhtMel", 0.1
            };
            interpolateTo[] = {};
        };
        class Hum_AbsStaCom_StgHvyFwdLhtMel: KH_MeleeBespokeBase
        {
            speed = 0.9375;
            minPlayTime = 1;
            soundOverride = "sprint";
            soundEdge[] = {0.85};
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_StgHvyFwdLhtMel.rtm";
            forceAim = 1;
            connectTo[] = {
                "Hum_AbsStaCom_StgHvyFwdLhtMel_Tra_AbsStaCom_IdlLhtMel", 0.1
            };
            interpolateTo[] = {};
        };
        class Hum_AbsStaCom_StgHvyLwdLhtMel: KH_MeleeBespokeBase
        {
            speed = 0.810811;
            minPlayTime = 1;
            soundOverride = "sprint";
            soundEdge[] = {0.54, 0.78};
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_StgHvyLwdLhtMel.rtm";
            forceAim = 1;
            connectTo[] = {
                "Hum_AbsStaCom_StgHvyLwdLhtMel_Tra_AbsStaCom_IdlLhtMel", 0.1
            };
            interpolateTo[] = {};
        };
        class Hum_AbsStaCom_StgHvyRwdLhtMel: KH_MeleeBespokeBase
        {
            speed = 0.909091;
            minPlayTime = 1;
            soundOverride = "sprint";
            soundEdge[] = {0.45, 0.78};
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_StgHvyRwdLhtMel.rtm";
            forceAim = 1;
            connectTo[] = {
                "Hum_AbsStaCom_StgHvyRwdLhtMel_Tra_AbsStaCom_IdlLhtMel", 0.1
            };
            interpolateTo[] = {};
        };
        class Hum_AbsStaCom_StgLhtBwdLhtMel: KH_MeleeBespokeBase
        {
            speed = 0.588235;
            minPlayTime = 0.9;
            soundOverride = "Walk";
            soundEdge[] = {0.15, 0.5, 0.76};
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_StgLhtBwdLhtMel.rtm";
            forceAim = 1;
            connectTo[] = {};
            interpolateTo[] = {
                "Hum_AbsStaCom_IdlLhtMel", 0.1
            };
        };
        class Hum_AbsStaCom_StgLhtFwdLhtMel: KH_MeleeBespokeBase
        {
            speed = 0.491803;
            minPlayTime = 0.9;
            soundOverride = "Walk";
            soundEdge[] = {0.15, 0.4, 0.65};
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_StgLhtFwdLhtMel.rtm";
            forceAim = 1;
            connectTo[] = {};
            interpolateTo[] = {
                "Hum_AbsStaCom_IdlLhtMel", 0.1
            };
        };
        class Hum_AbsStaCom_StgLhtLwdLhtMel: KH_MeleeBespokeBase
        {
            speed = 0.731707;
            minPlayTime = 0.9;
            soundOverride = "Walk";
            soundEdge[] = {0.31, 0.58, 0.82};
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_StgLhtLwdLhtMel.rtm";
            forceAim = 1;
            connectTo[] = {};
            interpolateTo[] = {
                "Hum_AbsStaCom_IdlLhtMel", 0.1
            };
        };
        class Hum_AbsStaCom_StgLhtRwdLhtMel: KH_MeleeBespokeBase
        {
            speed = 0.666667;
            minPlayTime = 0.9;
            soundOverride = "Walk";
            soundEdge[] = {0.26, 0.57, 0.88};
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_StgLhtRwdLhtMel.rtm";
            forceAim = 1;
            connectTo[] = {};
            interpolateTo[] = {
                "Hum_AbsStaCom_IdlLhtMel", 0.1
            };
        };
        class Hum_AbsStaCom_TklLhtMel: KH_MeleeBespokeBase
        {
            speed = 0.588235;
            kh_meleeTackleTiming[] = {
                {0.3921, 0.6274, "Tackle01"}
            };
            soundOverride = "Walk";
            soundEdge[] = {0.5, 0.64};
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_TklLhtMel.rtm";
            minPlayTime = 0.8;
            connectTo[] = {
                "Hum_AbsStaCom_IdlLhtMel", 0.1
            };
            interpolateTo[] = {
                "Hum_AbsStaCom_DgeBwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeLwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeRwdLhtMel", 0.1,
                "Hum_AbsStaCom_AttLhtMel01A", 0.1,
                "Hum_AbsStaCom_AttLhtMel02A", 0.1,
                "Hum_AbsStaCom_JogBwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkRwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TrnLftLhtMel", 0.1,
                "Hum_AbsStaCom_TrnRhtLhtMel", 0.1
            };
        };
        class Hum_AbsStaCom_TrnLftLhtMel: KH_MeleeBase
        {
            speed = 0.75;
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_TrnLftLhtMel.rtm";
            interpolationRestart = 1;
            connectTo[] = {};
            interpolateTo[] = {
                "Hum_AbsStaCom_JogBwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkRwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroRwdLhtMel", 0.1,
                "Hum_AbsStaCom_IdlLhtMel", 0.1,
                "Hum_AbsStaCom_TrnRhtLhtMel", 0.1,
                "Hum_AbsStaCom_AttLhtMel01A", 0.1,
                "Hum_AbsStaCom_AttLhtMel02A", 0.1,
                "Hum_AbsStaCom_DgeBwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeFwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeLwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TklLhtMel", 0.1
            };
        };
        class Hum_AbsStaCom_TrnRhtLhtMel: KH_MeleeBase
        {
            speed = 0.75;
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_TrnRhtLhtMel.rtm";
            interpolationRestart = 1;
            connectTo[] = {};
            interpolateTo[] = {
                "Hum_AbsStaCom_JogBwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkRwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroRwdLhtMel", 0.1,
                "Hum_AbsStaCom_IdlLhtMel", 0.1,
                "Hum_AbsStaCom_TrnLftLhtMel", 0.1,
                "Hum_AbsStaCom_AttLhtMel01A", 0.1,
                "Hum_AbsStaCom_AttLhtMel02A", 0.1,
                "Hum_AbsStaCom_DgeBwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeFwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeLwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TklLhtMel", 0.1
            };
        };
        class Hum_AbsStaCom_TroBwdLhtMel: KH_MeleeBase
        {
            actions = "KH_MeleeTertiaryLightSwordBackward";
            speed = 0.731707;
            Walkcycles = 2;
            soundEdge[] = {0.25, 0.5, 0.75, 1};
            soundOverride = "Tactical";
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_TroBwdLhtMel.rtm";
            connectTo[] = {};
            interpolateTo[] = {
                "Hum_AbsStaCom_JogBwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkRwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroRwdLhtMel", 0.1,
                "Hum_AbsStaCom_IdlLhtMel", 0.1,
                "Hum_AbsStaCom_TrnLftLhtMel", 0.1,
                "Hum_AbsStaCom_TrnRhtLhtMel", 0.1,
                "Hum_AbsStaCom_AttLhtMel01A", 0.1,
                "Hum_AbsStaCom_AttLhtMel02A", 0.1,
                "Hum_AbsStaCom_DgeBwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeFwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeLwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TklLhtMel", 0.1
            };
        };
        class Hum_AbsStaCom_TroBwdLwdLhtMel: KH_MeleeBase
        {
            actions = "KH_MeleeTertiaryLightSwordBackward";
            speed = 0.731707;
            Walkcycles = 2;
            soundEdge[] = {0.25, 0.5, 0.75, 1};
            soundOverride = "Tactical";
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_TroBwdLwdLhtMel.rtm";
            connectTo[] = {};
            interpolateTo[] = {
                "Hum_AbsStaCom_JogBwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkRwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroRwdLhtMel", 0.1,
                "Hum_AbsStaCom_IdlLhtMel", 0.1,
                "Hum_AbsStaCom_TrnLftLhtMel", 0.1,
                "Hum_AbsStaCom_TrnRhtLhtMel", 0.1,
                "Hum_AbsStaCom_AttLhtMel01A", 0.1,
                "Hum_AbsStaCom_AttLhtMel02A", 0.1,
                "Hum_AbsStaCom_DgeBwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeFwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeLwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TklLhtMel", 0.1
            };
        };
        class Hum_AbsStaCom_TroBwdRwdLhtMel: KH_MeleeBase
        {
            actions = "KH_MeleeTertiaryLightSwordBackward";
            speed = 0.731707;
            Walkcycles = 2;
            soundEdge[] = {0.25, 0.5, 0.75, 1};
            soundOverride = "Tactical";
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_TroBwdRwdLhtMel.rtm";
            connectTo[] = {};
            interpolateTo[] = {
                "Hum_AbsStaCom_JogBwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkRwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroRwdLhtMel", 0.1,
                "Hum_AbsStaCom_IdlLhtMel", 0.1,
                "Hum_AbsStaCom_TrnLftLhtMel", 0.1,
                "Hum_AbsStaCom_TrnRhtLhtMel", 0.1,
                "Hum_AbsStaCom_AttLhtMel01A", 0.1,
                "Hum_AbsStaCom_AttLhtMel02A", 0.1,
                "Hum_AbsStaCom_DgeBwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeFwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeLwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TklLhtMel", 0.1
            };
        };
        class Hum_AbsStaCom_TroFwdLhtMel: KH_MeleeBase
        {
            actions = "KH_MeleeTertiaryLightSwordForward";
            speed = 0.731707;
            Walkcycles = 2;
            soundEdge[] = {0.25, 0.5, 0.75, 1};
            soundOverride = "Tactical";
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_TroFwdLhtMel.rtm";
            connectTo[] = {};
            interpolateTo[] = {
                "Hum_AbsStaCom_JogBwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkRwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroRwdLhtMel", 0.1,
                "Hum_AbsStaCom_IdlLhtMel", 0.1,
                "Hum_AbsStaCom_TrnLftLhtMel", 0.1,
                "Hum_AbsStaCom_TrnRhtLhtMel", 0.1,
                "Hum_AbsStaCom_AttLhtMel01A", 0.1,
                "Hum_AbsStaCom_AttLhtMel02A", 0.1,
                "Hum_AbsStaCom_DgeBwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeFwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeLwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TklLhtMel", 0.1
            };
        };
        class Hum_AbsStaCom_TroFwdLwdLhtMel: KH_MeleeBase
        {
            actions = "KH_MeleeTertiaryLightSwordForward";
            speed = 0.731707;
            Walkcycles = 2;
            soundEdge[] = {0.25, 0.5, 0.75, 1};
            soundOverride = "Tactical";
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_TroFwdLwdLhtMel.rtm";
            connectTo[] = {};
            interpolateTo[] = {
                "Hum_AbsStaCom_JogBwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkRwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroRwdLhtMel", 0.1,
                "Hum_AbsStaCom_IdlLhtMel", 0.1,
                "Hum_AbsStaCom_TrnLftLhtMel", 0.1,
                "Hum_AbsStaCom_TrnRhtLhtMel", 0.1,
                "Hum_AbsStaCom_AttLhtMel01A", 0.1,
                "Hum_AbsStaCom_AttLhtMel02A", 0.1,
                "Hum_AbsStaCom_DgeBwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeFwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeLwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TklLhtMel", 0.1
            };
        };
        class Hum_AbsStaCom_TroFwdRwdLhtMel: KH_MeleeBase
        {
            actions = "KH_MeleeTertiaryLightSwordForward";
            speed = 0.731707;
            Walkcycles = 2;
            soundEdge[] = {0.25, 0.5, 0.75, 1};
            soundOverride = "Tactical";
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_TroFwdRwdLhtMel.rtm";
            connectTo[] = {};
            interpolateTo[] = {
                "Hum_AbsStaCom_JogBwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkRwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroRwdLhtMel", 0.1,
                "Hum_AbsStaCom_IdlLhtMel", 0.1,
                "Hum_AbsStaCom_TrnLftLhtMel", 0.1,
                "Hum_AbsStaCom_TrnRhtLhtMel", 0.1,
                "Hum_AbsStaCom_AttLhtMel01A", 0.1,
                "Hum_AbsStaCom_AttLhtMel02A", 0.1,
                "Hum_AbsStaCom_DgeBwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeFwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeLwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TklLhtMel", 0.1
            };
        };
        class Hum_AbsStaCom_TroLwdLhtMel: KH_MeleeBase
        {
            actions = "KH_MeleeTertiaryLightSwordLeftward";
            speed = 0.666667;
            Walkcycles = 2;
            soundEdge[] = {0.25, 0.5, 0.75, 1};
            soundOverride = "Tactical";
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_TroLwdLhtMel.rtm";
            connectTo[] = {};
            interpolateTo[] = {
                "Hum_AbsStaCom_JogBwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkRwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroRwdLhtMel", 0.1,
                "Hum_AbsStaCom_IdlLhtMel", 0.1,
                "Hum_AbsStaCom_TrnLftLhtMel", 0.1,
                "Hum_AbsStaCom_TrnRhtLhtMel", 0.1,
                "Hum_AbsStaCom_AttLhtMel01A", 0.1,
                "Hum_AbsStaCom_AttLhtMel02A", 0.1,
                "Hum_AbsStaCom_DgeBwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeFwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeLwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TklLhtMel", 0.1
            };
        };
        class Hum_AbsStaCom_TroRwdLhtMel: KH_MeleeBase
        {
            actions = "KH_MeleeTertiaryLightSwordRightward";
            speed = 0.75;
            Walkcycles = 2;
            soundEdge[] = {0.25, 0.5, 0.75, 1};
            soundOverride = "Tactical";
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_TroRwdLhtMel.rtm";
            connectTo[] = {};
            interpolateTo[] = {
                "Hum_AbsStaCom_JogBwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkRwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroLwdLhtMel", 0.1,
                "Hum_AbsStaCom_IdlLhtMel", 0.1,
                "Hum_AbsStaCom_TrnLftLhtMel", 0.1,
                "Hum_AbsStaCom_TrnRhtLhtMel", 0.1,
                "Hum_AbsStaCom_AttLhtMel01A", 0.1,
                "Hum_AbsStaCom_AttLhtMel02A", 0.1,
                "Hum_AbsStaCom_DgeBwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeFwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeLwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TklLhtMel", 0.1
            };
        };
        class Hum_AbsStaCom_WlkBwdLhtMel: KH_MeleeBase
        {
            actions = "KH_MeleeTertiaryLightSwordBackward";
            speed = 0.697674;
            Walkcycles = 2;
            soundEdge[] = {0.25, 0.5, 0.75, 1};
            soundOverride = "Walk";
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_WlkBwdLhtMel.rtm";
            connectTo[] = {};
            interpolateTo[] = {
                "Hum_AbsStaCom_JogBwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkRwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroRwdLhtMel", 0.1,
                "Hum_AbsStaCom_IdlLhtMel", 0.1,
                "Hum_AbsStaCom_TrnLftLhtMel", 0.1,
                "Hum_AbsStaCom_TrnRhtLhtMel", 0.1,
                "Hum_AbsStaCom_AttLhtMel01A", 0.1,
                "Hum_AbsStaCom_AttLhtMel02A", 0.1,
                "Hum_AbsStaCom_DgeBwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeFwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeLwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TklLhtMel", 0.1
            };
        };
        class Hum_AbsStaCom_WlkBwdLwdLhtMel: KH_MeleeBase
        {
            actions = "KH_MeleeTertiaryLightSwordBackward";
            speed = 0.697674;
            Walkcycles = 2;
            soundEdge[] = {0.25, 0.5, 0.75, 1};
            soundOverride = "Walk";
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_WlkBwdLwdLhtMel.rtm";
            connectTo[] = {};
            interpolateTo[] = {
                "Hum_AbsStaCom_JogBwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkRwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroRwdLhtMel", 0.1,
                "Hum_AbsStaCom_IdlLhtMel", 0.1,
                "Hum_AbsStaCom_TrnLftLhtMel", 0.1,
                "Hum_AbsStaCom_TrnRhtLhtMel", 0.1,
                "Hum_AbsStaCom_AttLhtMel01A", 0.1,
                "Hum_AbsStaCom_AttLhtMel02A", 0.1,
                "Hum_AbsStaCom_DgeBwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeFwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeLwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TklLhtMel", 0.1
            };
        };
        class Hum_AbsStaCom_WlkBwdRwdLhtMel: KH_MeleeBase
        {
            actions = "KH_MeleeTertiaryLightSwordBackward";
            speed = 0.697674;
            Walkcycles = 2;
            soundEdge[] = {0.25, 0.5, 0.75, 1};
            soundOverride = "Walk";
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_WlkBwdRwdLhtMel.rtm";
            connectTo[] = {};
            interpolateTo[] = {
                "Hum_AbsStaCom_JogBwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkRwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroRwdLhtMel", 0.1,
                "Hum_AbsStaCom_IdlLhtMel", 0.1,
                "Hum_AbsStaCom_TrnLftLhtMel", 0.1,
                "Hum_AbsStaCom_TrnRhtLhtMel", 0.1,
                "Hum_AbsStaCom_AttLhtMel01A", 0.1,
                "Hum_AbsStaCom_AttLhtMel02A", 0.1,
                "Hum_AbsStaCom_DgeBwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeFwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeLwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TklLhtMel", 0.1
            };
        };
        class Hum_AbsStaCom_WlkFwdLhtMel: KH_MeleeBase
        {
            actions = "KH_MeleeTertiaryLightSwordForward";
            speed = 1.15;
            soundOverride = "Walk";
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_WlkFwdLhtMel.rtm";
            connectTo[] = {};
            interpolateTo[] = {
                "Hum_AbsStaCom_JogBwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkRwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroRwdLhtMel", 0.1,
                "Hum_AbsStaCom_IdlLhtMel", 0.1,
                "Hum_AbsStaCom_TrnLftLhtMel", 0.1,
                "Hum_AbsStaCom_TrnRhtLhtMel", 0.1,
                "Hum_AbsStaCom_AttLhtMel01A", 0.1,
                "Hum_AbsStaCom_AttLhtMel02A", 0.1,
                "Hum_AbsStaCom_DgeBwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeFwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeLwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TklLhtMel", 0.1
            };
        };
        class Hum_AbsStaCom_WlkFwdLwdLhtMel: KH_MeleeBase
        {
            actions = "KH_MeleeTertiaryLightSwordForward";
            speed = 1.07143;
            soundOverride = "Walk";
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_WlkFwdLwdLhtMel.rtm";
            connectTo[] = {};
            interpolateTo[] = {
                "Hum_AbsStaCom_JogBwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkRwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroRwdLhtMel", 0.1,
                "Hum_AbsStaCom_IdlLhtMel", 0.1,
                "Hum_AbsStaCom_TrnLftLhtMel", 0.1,
                "Hum_AbsStaCom_TrnRhtLhtMel", 0.1,
                "Hum_AbsStaCom_AttLhtMel01A", 0.1,
                "Hum_AbsStaCom_AttLhtMel02A", 0.1,
                "Hum_AbsStaCom_DgeBwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeFwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeLwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TklLhtMel", 0.1
            };
        };
        class Hum_AbsStaCom_WlkFwdRwdLhtMel: KH_MeleeBase
        {
            actions = "KH_MeleeTertiaryLightSwordForward";
            speed = 1.07143;
            soundOverride = "Walk";
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_WlkFwdRwdLhtMel.rtm";
            connectTo[] = {};
            interpolateTo[] = {
                "Hum_AbsStaCom_JogBwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkRwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroRwdLhtMel", 0.1,
                "Hum_AbsStaCom_IdlLhtMel", 0.1,
                "Hum_AbsStaCom_TrnLftLhtMel", 0.1,
                "Hum_AbsStaCom_TrnRhtLhtMel", 0.1,
                "Hum_AbsStaCom_AttLhtMel01A", 0.1,
                "Hum_AbsStaCom_AttLhtMel02A", 0.1,
                "Hum_AbsStaCom_DgeBwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeFwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeLwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TklLhtMel", 0.1
            };
        };
        class Hum_AbsStaCom_WlkLwdLhtMel: KH_MeleeBase
        {
            actions = "KH_MeleeTertiaryLightSwordLeftward";
            speed = 0.625;
            Walkcycles = 2;
            soundEdge[] = {0.25, 0.5, 0.75, 1};
            soundOverride = "Walk";
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_WlkLwdLhtMel.rtm";
            connectTo[] = {};
            interpolateTo[] = {
                "Hum_AbsStaCom_JogBwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkRwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroRwdLhtMel", 0.1,
                "Hum_AbsStaCom_IdlLhtMel", 0.1,
                "Hum_AbsStaCom_TrnLftLhtMel", 0.1,
                "Hum_AbsStaCom_TrnRhtLhtMel", 0.1,
                "Hum_AbsStaCom_AttLhtMel01A", 0.1,
                "Hum_AbsStaCom_AttLhtMel02A", 0.1,
                "Hum_AbsStaCom_DgeBwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeFwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeLwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TklLhtMel", 0.1
            };
        };
        class Hum_AbsStaCom_WlkRwdLhtMel: KH_MeleeBase
        {
            actions = "KH_MeleeTertiaryLightSwordRightward";
            speed = 0.625;
            Walkcycles = 2;
            soundEdge[] = {0.25, 0.5, 0.75, 1};
            soundOverride = "Walk";
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_WlkRwdLhtMel.rtm";
            connectTo[] = {};
            interpolateTo[] = {
                "Hum_AbsStaCom_JogBwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkLwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroRwdLhtMel", 0.1,
                "Hum_AbsStaCom_IdlLhtMel", 0.1,
                "Hum_AbsStaCom_TrnLftLhtMel", 0.1,
                "Hum_AbsStaCom_TrnRhtLhtMel", 0.1,
                "Hum_AbsStaCom_AttLhtMel01A", 0.1,
                "Hum_AbsStaCom_AttLhtMel02A", 0.1,
                "Hum_AbsStaCom_DgeBwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeFwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeLwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TklLhtMel", 0.1
            };
        };
        class Hum_AbsStaSaf_IdlLhtMel: KH_MeleeBase
        {
            canBlendStep = 0;
            speed = 1e+10;
            file = "x\kh\addons\main\animations\human\Hum_AbsStaSaf_IdlLhtMel.rtm";
            connectTo[] = {
                "Hum_AbsStaSaf_IdlLhtMel_Tra_AbsStaCom_IdlLhtMel", 0.1
            };
            interpolateTo[] += {
                "Hum_AbsStaSaf_IdlLhtMel_Tra_AbsStaSaf_IdlUna", 0.1
            };
        };
        class Hum_AbsStaCom_AttLhtMel01A_Tra_AbsStaCom_IdlLhtMel: KH_MeleeBespokeBase
        {
            speed = 1.7857125;
            minPlayTime = 1;
            soundOverride = "Walk";
            soundEdge[] = {0.8};
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_AttLhtMel01A_Tra_AbsStaCom_IdlLhtMel.rtm";
            connectTo[] = {
                "Hum_AbsStaCom_IdlLhtMel", 0.1
            };
            interpolateTo[] = {
                "Hum_AbsStaCom_DgeBwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeFwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeLwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeRwdLhtMel", 0.1,
                "Hum_AbsStaCom_RclRhtLhtMel", 0.15,
                "Hum_AbsStaCom_JogBwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkRwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TrnLftLhtMel", 0.1,
                "Hum_AbsStaCom_TrnRhtLhtMel", 0.1,
                "Hum_AbsStaCom_TklLhtMel", 0.1
            };
        };
        class Hum_AbsStaCom_AttLhtMel01B_Tra_AbsStaCom_IdlLhtMel: KH_MeleeBespokeBase
        {
            speed = 1.7857125;
            minPlayTime = 0.6;
            soundOverride = "Walk";
            soundEdge[] = {0.66, 0.99};
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_AttLhtMel01B_Tra_AbsStaCom_IdlLhtMel.rtm";
            connectTo[] = {
                "Hum_AbsStaCom_IdlLhtMel", 0.1
            };
            interpolateTo[] = {
                "Hum_AbsStaCom_DgeBwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeFwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeLwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeRwdLhtMel", 0.1,
                "Hum_AbsStaCom_RclRhtLhtMel", 0.15,
                "Hum_AbsStaCom_JogBwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkRwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TrnLftLhtMel", 0.1,
                "Hum_AbsStaCom_TrnRhtLhtMel", 0.1,
                "Hum_AbsStaCom_TklLhtMel", 0.1
            };
        };
        class Hum_AbsStaCom_AttLhtMel02A_Tra_AbsStaCom_IdlLhtMel: KH_MeleeBespokeBase
        {
            speed = 1.3888875;
            minPlayTime = 0.75;
            soundOverride = "Walk";
            soundEdge[] = {0.75, 0.99};
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_AttLhtMel02A_Tra_AbsStaCom_IdlLhtMel.rtm";
            connectTo[] = {
                "Hum_AbsStaCom_IdlLhtMel", 0.1
            };
            interpolateTo[] = {
                "Hum_AbsStaCom_DgeBwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeFwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeLwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeRwdLhtMel", 0.1,
                "Hum_AbsStaCom_RclRhtLhtMel", 0.15,
                "Hum_AbsStaCom_JogBwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkRwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TrnLftLhtMel", 0.1,
                "Hum_AbsStaCom_TrnRhtLhtMel", 0.1,
                "Hum_AbsStaCom_TklLhtMel", 0.1
            };
        };
        class Hum_AbsStaCom_AttLhtMel02B_Tra_AbsStaCom_IdlLhtMel: KH_MeleeBespokeBase
        {
            speed = 0.56818125;
            minPlayTime = 0.6;
            soundOverride = "Walk";
            soundEdge[] = {0.21, 0.5, 0.8};
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_AttLhtMel02B_Tra_AbsStaCom_IdlLhtMel.rtm";
            connectTo[] = {
                "Hum_AbsStaCom_IdlLhtMel", 0.1
            };
            interpolateTo[] = {
                "Hum_AbsStaCom_DgeBwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeFwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeLwdLhtMel", 0.1,
                "Hum_AbsStaCom_DgeRwdLhtMel", 0.1,
                "Hum_AbsStaCom_RclRhtLhtMel", 0.15,
                "Hum_AbsStaCom_JogBwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogLwdLhtMel", 0.1,
                "Hum_AbsStaCom_JogRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkLwdLhtMel", 0.1,
                "Hum_AbsStaCom_WlkRwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_RunFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroBwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroFwdRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroLwdLhtMel", 0.1,
                "Hum_AbsStaCom_TroRwdLhtMel", 0.1,
                "Hum_AbsStaCom_TrnLftLhtMel", 0.1,
                "Hum_AbsStaCom_TrnRhtLhtMel", 0.1,
                "Hum_AbsStaCom_TklLhtMel", 0.1
            };
        };
        class Hum_AbsStaCom_IdlLhtMel_Tra_AbsStaSaf_IdlLhtMel: KH_MeleeBespokeBase
        {
            speed = 1.875;
            minPlayTime = 0.5;
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_IdlLhtMel_Tra_AbsStaSaf_IdlLhtMel.rtm";
            connectTo[] = {
                "Hum_AbsStaSaf_IdlLhtMel", 0.1
            };
            interpolateTo[] = {};
        };
        class Hum_AbsStaCom_StgHvyBwdLhtMel_Tra_AbsStaCom_IdlLhtMel: KH_MeleeBespokeBase
        {
            speed = 0.348837;
            minPlayTime = 1;
            soundOverride = "Walk";
            soundEdge[] = {0.25, 0.5, 0.81, 0.9};
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_StgHvyBwdLhtMel_Tra_AbsStaCom_IdlLhtMel.rtm";
            connectTo[] = {
                "Hum_AbsStaCom_IdlLhtMel", 0.1
            };
            interpolateTo[] = {};
        };
        class Hum_AbsStaCom_StgHvyFwdLhtMel_Tra_AbsStaCom_IdlLhtMel: KH_MeleeBespokeBase
        {
            speed = 0.319149;
            minPlayTime = 1;
            soundOverride = "Walk";
            soundEdge[] = {0.54, 0.78, 0.93};
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_StgHvyFwdLhtMel_Tra_AbsStaCom_IdlLhtMel.rtm";
            connectTo[] = {
                "Hum_AbsStaCom_IdlLhtMel", 0.1
            };
            interpolateTo[] = {};
        };
        class Hum_AbsStaCom_StgHvyLwdLhtMel_Tra_AbsStaCom_IdlLhtMel: KH_MeleeBespokeBase
        {
            speed = 0.340909;
            minPlayTime = 1;
            soundOverride = "Walk";
            soundEdge[] = {0.48, 0.72, 0.9};
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_StgHvyLwdLhtMel_Tra_AbsStaCom_IdlLhtMel.rtm";
            connectTo[] = {
                "Hum_AbsStaCom_IdlLhtMel", 0.1
            };
            interpolateTo[] = {};
        };
        class Hum_AbsStaCom_StgHvyRwdLhtMel_Tra_AbsStaCom_IdlLhtMel: KH_MeleeBespokeBase
        {
            speed = 0.361446;
            minPlayTime = 1;
            soundOverride = "Walk";
            soundEdge[] = {0.6, 0.72, 0.87};
            file = "x\kh\addons\main\animations\human\Hum_AbsStaCom_StgHvyRwdLhtMel_Tra_AbsStaCom_IdlLhtMel.rtm";
            connectTo[] = {
                "Hum_AbsStaCom_IdlLhtMel", 0.1
            };
            interpolateTo[] = {};
        };
        class Hum_AbsStaSaf_IdlLhtMel_Tra_AbsStaCom_IdlLhtMel: KH_MeleeBespokeBase
        {
            speed = 1.875;
            minPlayTime = 0.5;
            file = "x\kh\addons\main\animations\human\Hum_AbsStaSaf_IdlLhtMel_Tra_AbsStaCom_IdlLhtMel.rtm";
            connectTo[] = {};
            interpolateTo[] = {
                "Hum_AbsStaCom_IdlLhtMel", 0.1
            };
        };
        class Hum_AbsStaSaf_IdlLhtMel_Tra_AbsStaSaf_IdlUna: KH_MeleeBespokeBase
        {
            speed = 0.967742;
            minPlayTime = 1;
            file = "x\kh\addons\main\animations\human\Hum_AbsStaSaf_IdlLhtMel_Tra_AbsStaSaf_IdlUna.rtm";
            rightHandIKBeg = 0;
            rightHandIKCurve[] = {0};
            rightHandIKEnd = 0;
            connectTo[] = {};
            interpolateTo[] = {
                "AmovPercMstpSnonWnonDnon", 0.5
            };
        };
        class Hum_AbsStaSaf_IdlUna_Tra_AbsStaSaf_IdlLhtMel: KH_MeleeBespokeBase
        {
            speed = 0.967742;
            minPlayTime = 1;
            file = "x\kh\addons\main\animations\human\Hum_AbsStaSaf_IdlUna_Tra_AbsStaSaf_IdlLhtMel.rtm";
            rightHandIKBeg = 0;
            rightHandIKCurve[] = {0};
            rightHandIKEnd = 0;
            connectTo[] = {};
            interpolateTo[] = {
                "Hum_AbsStaSaf_IdlLhtMel", 0.1
            };
        };
        class AmovPercMstpSnonWnonDnon: StandBase
        {
            interpolateTo[] += {
                "Hum_AbsStaSaf_IdlUna_Tra_AbsStaSaf_IdlLhtMel", 0.01
            };
        };
        class AmovPknlMstpSrasWlnrDnon_AinvPknlMstpSrasWlnrDnon;
        class AinvPknlMstpSlayWlnrDnon_medicOther: AmovPknlMstpSrasWlnrDnon_AinvPknlMstpSrasWlnrDnon
        {
            forceAim = 1;
        };
        class AmovPknlMstpSnonWnonDnon;
        class AinvPknlMstpSlayWnonDnon_medic: AmovPknlMstpSnonWnonDnon
        {
            forceAim = 1;
        };
        class AmovPknlMstpSrasWrflDnon_AinvPknlMstpSlayWrflDnon;
        class AinvPknlMstpSlayWnonDnon_medicOther: AmovPknlMstpSrasWrflDnon_AinvPknlMstpSlayWrflDnon
        {
            forceAim = 1;
        };
        class AmovPknlMstpSrasWpstDnon;
        class AinvPknlMstpSlayWpstDnon_medic: AmovPknlMstpSrasWpstDnon
        {
            forceAim = 1;
        };
        class AinvPknlMstpSlayWpstDnon_medicOther: AmovPknlMstpSrasWpstDnon
        {
            forceAim = 1;
        };
        class AinvPknlMstpSlayWrflDnon_medic: AmovPknlMstpSrasWrflDnon_AinvPknlMstpSlayWrflDnon
        {
            forceAim = 1;
        };
        class AinvPknlMstpSlayWrflDnon_medicOther: AmovPknlMstpSrasWrflDnon_AinvPknlMstpSlayWrflDnon
        {
            forceAim = 1;
        };
        class AinvPpneMstpSlayWnonDnon_medic: AinvPknlMstpSlayWpstDnon_medic
        {
            forceAim = 1;
        };
        class AinvPpneMstpSlayWnonDnon_medicOther: AinvPknlMstpSlayWnonDnon_medicOther
        {
            forceAim = 1;
        };
        class AinvPpneMstpSlayWpstDnon_medic: AinvPknlMstpSlayWpstDnon_medic
        {
            forceAim = 1;
        };
        class AinvPpneMstpSlayWpstDnon_medicOther: AinvPknlMstpSlayWpstDnon_medicOther
        {
            forceAim = 1;
        };
        class AinvPpneMstpSlayWrflDnon_medic: AinvPknlMstpSlayWrflDnon_medic
        {
            forceAim = 1;
        };
        class AinvPpneMstpSlayWrflDnon_medicOther: AinvPknlMstpSlayWrflDnon_medic
        {
            forceAim = 1;
        };
    };
};