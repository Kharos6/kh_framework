class CfgAmmo
{
	class BulletBase;
	class KH_MeleeBase: BulletBase
	{
		airFriction = -0.00001;
		cartridge = "";
		caliber = 6;
		cost = 15;
		explosive = 0;
		hit = 30;
		typicalSpeed = 1200;
	};
	class KH_MeleeGenericManSwing: KH_MeleeBase
	{
		caliber = 6;
		hit = 30;
	};
	class KH_MeleeGenericManThrust: KH_MeleeBase
	{
		caliber = 6;
		hit = 30;
	};
	class KH_MeleeGenericManKick: KH_MeleeBase
	{
		caliber = 6;
		hit = 30;
	};
	class KH_MeleeGenericManTackle: KH_MeleeBase
	{
		caliber = 6;
		hit = 30;
	};
	class KH_MeleeCleavingSharpLightSwing: KH_MeleeBase
	{
		caliber = 6;
		hit = 30;
	};
	class KH_MeleeCleavingSharpLightThrust: KH_MeleeBase
	{
		caliber = 6;
		hit = 30;
	};
	class KH_MeleeBluntLightSwing: KH_MeleeBase
	{
		caliber = 3;
		hit = 30;
	};
	class KH_MeleeBluntLightThrust: KH_MeleeBase
	{
		caliber = 3;
		hit = 30;
	};
    class MissileBase;
    class KH_MeleeDummy: MissileBase {};
};