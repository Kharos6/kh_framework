class CfgWeapons
{
    class Default
    {
        kh_meleeMoves = "";
        kh_meleeType = "KH_MeleeGenericBluntLight";
    };
    class Launcher_Base_F;
	class KH_MeleeWeaponTertiaryBase: Launcher_Base_F
	{
		author = "Kharos";
        drySound[] = {"", 1, 1, 1};
        descriptionShort = "Melee Weapon";
        icon = "x\kh\addons\main\ui\kh_logo_512.paa";
        value = 20;
        magazines[] = {"KH_MeleeGeneric"};
        magazineWell[] = {"KH_MeleeGeneric"};
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
        class Swing
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
            displayName = "Swing";
            ffCount = 1;
            ffFrequency = 11;
            ffMagnitude = 0.5;
            flash = "gunfire";
            flashSize = 0.1;
            maxRange = 790;
            maxRangeProbab = 0.8;
            midRange = 50;
            midRangeProbab = 0.8;
            minRange = 25;
            minRangeProbab = 0.8;
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
        class Thrust
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
            displayName = "Thrust";
            ffCount = 1;
            ffFrequency = 11;
            ffMagnitude = 0.5;
            flash = "gunfire";
            flashSize = 0.1;
            maxRange = 790;
            maxRangeProbab = 0.8;
            midRange = 50;
            midRangeProbab = 0.8;
            minRange = 25;
            minRangeProbab = 0.8;
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
            textureType = "semi";
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
		scope = 2;
		displayName = "Sword - Generic 01";
		handAnim[] = {"OFP2_ManSkeleton", "\x\kh\addons\main\animations\human\Hum_AbsStaSaf_IdlLhtMel.rtm"};
        picture = "";
        kh_meleeMoves = "KH_MeleeInLightSword";
        kh_meleeType = "KH_MeleeCleavingSharpLight";
		model = "\x\kh\addons\main\models\SM_SwordGeneric01.p3d";
        modes[] = {"Swing", "Thrust"};
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
};