class CfgAmmo
{
	class BulletBase;
	class KH_MeleeBase: BulletBase
	{
		airFriction = -0.00001;
		audibleFire = 0;
		craterEffects = "";
		craterWaterEffects = "";
		bulletFly[] = {};
		bulletFly1[] = {};
		bulletFly10[] = {};
		bulletFly11[] = {};
		bulletFly12[] = {};
		bulletFly2[] = {};
		bulletFly3[] = {};
		bulletFly4[] = {};
		bulletFly5[] = {};
		bulletFly6[] = {};
		bulletFly7[] = {};
		bulletFly8[] = {};
		bulletFly9[] = {};
		cartridge = "";
		caliber = 1;
        aiAmmoUsageFlags = 0;
        allowAgainstInfantry = 0;
        cost = 999999;
		explosive = 0;
		hit = 1;
		typicalSpeed = 1;
		soundFakeFall[] = {};
		soundImpactDefault1[] = {};
		soundSetBulletFly[] = {};
		soundSetSonicCrack[] = {};
		supersonicCrackFar[] = {};
		supersonicCrackNear[] = {};
		kh_melee = 1;
		kh_meleeActionType = "";
	};
	class KH_MeleeGenericManSwing: KH_MeleeBase
	{
		caliber = 1;
		hit = 1.5;
		kh_meleeActionType = "attack";
	};
	class KH_MeleeGenericManThrust: KH_MeleeBase
	{
		caliber = 2;
		hit = 1.2;
		kh_meleeActionType = "attack";
	};
	class KH_MeleeGenericManKick: KH_MeleeBase
	{
		caliber = 1;
		hit = 2;
		kh_meleeActionType = "kick";
	};
	class KH_MeleeGenericManTackle: KH_MeleeBase
	{
		caliber = 1;
		hit = 1.75;
		kh_meleeActionType = "tackle";
	};
	class KH_MeleeCleavingSharpLightLongSwing: KH_MeleeBase
	{
		caliber = 3;
		hit = 8;
		kh_meleeActionType = "attack";
	};
	class KH_MeleeCleavingSharpLightLongThrust: KH_MeleeBase
	{
		caliber = 4;
		hit = 5;
		kh_meleeActionType = "attack";
	};
	class KH_MeleeCleavingSharpLightShortSwing: KH_MeleeBase
	{
		caliber = 5;
		hit = 6.5;
		kh_meleeActionType = "attack";
	};
	class KH_MeleeCleavingSharpLightShortThrust: KH_MeleeBase
	{
		caliber = 6;
		hit = 5.5;
		kh_meleeActionType = "attack";
	};
	class KH_MeleeBluntLightSwing: KH_MeleeBase
	{
		caliber = 1;
		hit = 9;
		kh_meleeActionType = "attack";
	};
	class KH_MeleeBluntLightThrust: KH_MeleeBase
	{
		caliber = 2;
		hit = 6.5;
		kh_meleeActionType = "attack";
	};
    class MissileBase;
    class KH_MeleeDummy: MissileBase
	{
        effectsSmoke = "";
        explosionEffects = "";
        explosionSoundEffect = "";
        muzzleEffect = "";
	};
};