class CfgWeapons
{
    class Launcher_Base_F;
	class KH_MeleeWeaponTertiaryBase: Launcher_Base_F
	{
		author = "Kharos";
        drySound[] = {"", 1, 1, 1};
        descriptionShort = "Melee Weapon";
        value = 20;
        kh_meleeActions = "";
        kh_meleeType = "";
        kh_meleeModes[] = {};
        magazines[] = {"KH_MeleeGeneric"};
        magazineWell[] = {"KH_MeleeGeneric"};
        magazineReloadTime = 0;
        reloadAction = "";
        modes[] = {"DefaultMode"};
        class Eventhandlers {};
        class Library
        {
            libTextDesc = "A melee weapon.";
        };
        class ItemInfo
        {
            priority = 3;
        };
        class WeaponSlotsInfo
        {
            allowedSlots[] = {};
            mass = 100;
        };
        class DefaultMode
        {
            aiDispersionCoefX = 1.4;
            aiDispersionCoefY = 1.7;
            aiRateOfFire = 7;
            aiRateOfFireDispersion = 1;
            aiRateOfFireDistance = 500;
            artilleryCharge = 1;
            artilleryDispersion = 1;
            autoFire = 0;
            burst = 1;
            burstRangeMax = -1;
            canShootInWater = 0;
            dispersion = 0.0002;
            displayName = "DefaultMode";
            ffCount = 1;
            ffFrequency = 11;
            ffMagnitude = 0.5;
            flash = "";
            flashSize = 0;
            maxRange = 100;
            maxRangeProbab = 0;
            midRange = 50;
            midRangeProbab = 0;
            minRange = 1;
            minRangeProbab = 0;
            multiplier = 1;
            recoil = "";
            recoilProne = "";
            reloadTime = 0;
            requiredOpticType = -1;
            showToPlayer = 1;
            sound[] = {"", 10, 1};
            soundBegin[] = {"sound", 1};
            soundBeginWater[] = {"sound", 1};
            soundBurst = 0;
            soundClosure[] = {"sound", 1};
            soundContinuous = 0;
            soundEnd[] = {};
            soundLoop[] = {};
            sounds[] = {"StandardSound"};
            textureType = "fullAuto";
            useAction = 0;
            useActionTitle = "";
            weaponSoundEffect = "";
            class BaseSoundModeType {};
            class StandardSound
            {
                begin1[] = {"", 1, 1, 1};
                soundBegin[] = {"begin1", 1};
                soundSetShot[] = {""};
            };
        };
	};
	class KH_SwordGeneric01: KH_MeleeWeaponTertiaryBase
	{
        author = "Kharos";
		scope = 2;
		displayName = "Sword - Generic 01";
		handAnim[] = {"OFP2_ManSkeleton", "\x\kh\addons\main\animations\human\Hum_AbsStaSaf_IdlLhtMel.rtm"};
        overviewPicture = "\x\kh\addons\main\ui\sword_generic_01.paa";
        picture = "\x\kh\addons\main\ui\sword_generic_01.paa";
        kh_meleeActions = "KH_MeleeInLightSword";
        kh_meleeType = "KH_MeleeCleavingSharpLightLong";
        kh_meleeModes[] = {"KH_MeleeAttack01", "KH_MeleeAttack02"};
        kh_meleeModesGestures[] = {{"KH_MeleeAttack01AGesture", "KH_MeleeAttack01BGesture"}, {}};
		model = "\x\kh\addons\main\models\SM_SwordGeneric01.p3d";
		hiddenSelections[] = {
			"mat_sword"
		};
		hiddenSelectionsMaterials[] = {
			"x\kh\addons\main\materials\weapons\SwordGeneric01.rvmat"
		};
		hiddenSelectionsTextures[] = {
			"x\kh\addons\main\textures\weapons\SwordGeneric01_CO.paa"
		};
	};
	class KH_KnifeM3: KH_MeleeWeaponTertiaryBase
	{
        author = "Kharos";
		scope = 2;
		displayName = "Knife M3";
		handAnim[] = {"OFP2_ManSkeleton", "\x\kh\addons\main\animations\human\Hum_AbsStaSaf_IdlLhtMel.rtm"};
        overviewPicture = "\x\kh\addons\main\ui\knife_m3.paa";
        picture = "\x\kh\addons\main\ui\knife_m3.paa";
        kh_meleeActions = "KH_MeleeInLightSword";
        kh_meleeType = "KH_MeleeCleavingSharpLightShort";
        kh_meleeModes[] = {"KH_MeleeAttack01", "KH_MeleeAttack02"};
        kh_meleeModesGestures[] = {{"KH_MeleeAttack01AGesture", "KH_MeleeAttack01BGesture"}, {}};
		model = "\x\kh\addons\main\models\SM_KnifeM3.p3d";
		hiddenSelections[] = {
			"mat_knife"
		};
		hiddenSelectionsMaterials[] = {
			"x\kh\addons\main\materials\weapons\KnifeM3.rvmat"
		};
		hiddenSelectionsTextures[] = {
			"x\kh\addons\main\textures\weapons\KnifeM3_CO.paa"
		};
	};
};