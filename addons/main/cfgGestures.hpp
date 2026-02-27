class CfgGesturesMale
{
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
    class BlendAnims
    {
        kh_meleeUpperBody[] = {
            "head", 1,
            "neck1", 1,
            "neck", 1,
            "weapon", 1,
            "launcher", 1,
            "LeftShoulder", 1,
            "LeftArm", 1,
            "LeftArmRoll", 1,
            "LeftForeArm", 1,
            "LeftForeArmRoll",1 ,
            "LeftHand", 1,
            "LeftHandRing", 1,
            "LeftHandPinky1", 1,
            "LeftHandPinky2", 1,
            "LeftHandPinky3", 1,
            "LeftHandRing1", 1,
            "LeftHandRing2", 1,
            "LeftHandRing3", 1,
            "LeftHandMiddle1", 1,
            "LeftHandMiddle2", 1,
            "LeftHandMiddle3", 1,
            "LeftHandIndex1", 1,
            "LeftHandIndex2", 1,
            "LeftHandIndex3", 1,
            "LeftHandThumb1", 1,
            "LeftHandThumb2", 1,
            "LeftHandThumb3", 1,
            "RightShoulder", 1,
            "RightArm", 1,
            "RightArmRoll", 1,
            "RightForeArm", 1,
            "RightForeArmRoll", 1,
            "RightHand", 1,
            "RightHandRing", 1,
            "RightHandPinky1", 1,
            "RightHandPinky2", 1,
            "RightHandPinky3", 1,
            "RightHandRing1", 1,
            "RightHandRing2", 1,
            "RightHandRing3", 1,
            "RightHandMiddle1", 1,
            "RightHandMiddle2", 1,
            "RightHandMiddle3", 1,
            "RightHandIndex1", 1,
            "RightHandIndex2", 1,
            "RightHandIndex3", 1,
            "RightHandThumb1", 1,
            "RightHandThumb2", 1,
            "RightHandThumb3", 1,
            "Spine", 0.25,
            "Spine1", 0.75,
            "Spine2", 1,
            "Spine3", 1
        };
        kh_meleeHitReaction[] = {
            "head", 0.5,
            "neck1", 0.5,
            "neck", 0.5,
            "weapon", 0.5,
            "launcher", 0.5,
            "LeftShoulder", 0.5,
            "LeftArm", 0.5,
            "RightShoulder", 0.5,
            "RightArm", 0.5,
            "Spine", 0.1,
            "Spine1", 1,
            "Spine2", 1,
            "Spine3", 1
        };
    };
    class States
    {
        class GestureNone: Default
        {
            mask = "empty";
            looped = 0;
            speed = 1e+10;
        };
        class KH_MeleeTertiaryGestureBase: Default
        {
            actions = "NoActions";
            canPullTrigger = 0;
            canReload = 0;
            connectAs = "";
            connectFrom[] = {};
            connectTo[] = {};
            disableWeapons = 1;
            disableWeaponsLong = 1;
            enableBinocular = 0;
            enableMissile = 0;
            enableOptics = 0;
            equivalentTo = "";
            file = "";
            forceAim = 0;
            headBobMode = 0;
            headBobStrength = 0;
            interpolateFrom[] = {};
            interpolateTo[] = {};
            interpolateWith[] = {};
            interpolationRestart = 1;
            interpolationSpeed = 6;
            kh_melee = 1;
            leftHandIKBeg = 0;
            leftHandIKCurve[] = {0};
            leftHandIKEnd = 0;
            looped = 0;
            mask = "kh_meleeUpperBody";
            minPlayTime = 0;
            preload = 1;
            ragdoll = 0;
            relSpeedMax = 1;
            relSpeedMin = 1;
            rightHandIKBeg = 1;
            rightHandIKCurve[] = {1};
            rightHandIKEnd = 1;
            showHandGun = 0;
            showItemInHand = 0;
            showItemInRightHand = 0;
            showWeaponAim = 1;
            soundEdge[] = {};
            soundEnabled = 0;
            soundOverride = "";
            speed = 1e+10;
            static = 0;
            terminal = 0;
            Walkcycles = 1;
        };
        class Hum_AddStaCom_BloLhtMel: KH_MeleeTertiaryGestureBase
        {
            looped = 1;
            kh_meleeBlockTiming[] = {
                {0, 1, "Block01"}
            };
            file = "x\kh\addons\main\animations\human\Hum_AddStaCom_BloLhtMel.rtm";
        };
        class Hum_AddStaCom_ParLhtMel: KH_MeleeTertiaryGestureBase
        {
            speed = 0.697674;
            kh_meleeParryTiming[] = {
                {0, 1, "Parry01"}
            };
            file = "x\kh\addons\main\animations\human\Hum_AddStaCom_ParLhtMel.rtm";
        };
        class Hum_AddStaCom_AttLhtMel01A: KH_MeleeTertiaryGestureBase
        {
            minPlayTime = 0.66;
            kh_meleeHitTiming[] = {
                {0.4800, "launcher", {0.811733, 0.531356, 1.645111}, "Attack01", 0},
                {0.5000, "launcher", {0.664187, 0.771464, 1.378433}, "Attack01", 0},
                {0.5200, "launcher", {0.385124, 0.908587, 1.239768}, "Attack01", 0},
                {0.5400, "launcher", {0.045440, 0.922149, 1.209739}, "Attack01", 0}
            };
            kh_meleeSoundTiming[] = {
                {0.4800, "launcher", "Attack01", "swing"}
            };
            speed = 0.75;
            file = "x\kh\addons\main\animations\human\Hum_AddStaCom_AttLhtMel01A.rtm";
        };
        class Hum_AddStaCom_AttLhtMel01B: KH_MeleeTertiaryGestureBase
        {
            minPlayTime = 0.5609;
            kh_meleeHitTiming[] = {
                {0.3171, "launcher", {-0.251478, 0.386765, 1.867224}, "Attack01", 0},
                {0.3415, "launcher", {-0.167461, 0.621735, 1.699616}, "Attack01", 0},
                {0.3659, "launcher", {0.067644, 0.786245, 1.386941}, "Attack01", 0},
                {0.3902, "launcher", {0.445221, 0.692424, 1.026093}, "Attack01", 0},
                {0.4146, "launcher", {0.704612, 0.399408, 0.827776}, "Attack01", 0},
                {0.4390, "launcher", {0.812083, 0.175080, 0.819633}, "Attack01", 0}
            };
            kh_meleeSoundTiming[] = {
                {0.3171, "launcher", "Attack01", "swing"}
            };
            speed = 0.9146;
            file = "x\kh\addons\main\animations\human\Hum_AddStaCom_AttLhtMel01B.rtm";
        };
        class Hum_AddStaCom_BloSucLhtMel: KH_MeleeTertiaryGestureBase
        {
            kh_meleeBlockTiming[] = {
                {0, 1, "Block01"}
            };
            speed = 1.5789;
            file = "x\kh\addons\main\animations\human\Hum_AddStaCom_BloSucLhtMel.rtm";
        };
        class Hum_AddAnyAny_HitBckLhtMel: KH_MeleeTertiaryGestureBase
        {
            mask = "kh_meleeHitReaction";
            speed = 1.5;
            file = "x\kh\addons\main\animations\human\Hum_AddAnyAny_HitBckLhtMel.rtm";
        };
        class Hum_AddAnyAny_HitFroLhtMel: KH_MeleeTertiaryGestureBase
        {
            mask = "kh_meleeHitReaction";
            speed = 1.5;
            file = "x\kh\addons\main\animations\human\Hum_AddAnyAny_HitFroLhtMel.rtm";
        };
        class Hum_AddAnyAny_HitLftLhtMel: KH_MeleeTertiaryGestureBase
        {
            mask = "kh_meleeHitReaction";
            speed = 1.5;
            file = "x\kh\addons\main\animations\human\Hum_AddAnyAny_HitLftLhtMel.rtm";
        };
        class Hum_AddAnyAny_HitRhtLhtMel: KH_MeleeTertiaryGestureBase
        {
            mask = "kh_meleeHitReaction";
            speed = 1.5;
            file = "x\kh\addons\main\animations\human\Hum_AddAnyAny_HitRhtLhtMel.rtm";
        };
    };
};