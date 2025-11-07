class CfgVehicles
{
	class Logic;
	class Module_F: Logic
	{
		class AttributesBase
		{
			class Checkbox;
			class CheckboxNumber;
			class Combo;
			class Default;
			class Edit;
			class EditCodeMulti5 {
				control = "EditCodeMulti5";
				defaultValue = "''";
				expression = "_this setVariable ['%s', _value, true];";
			};
			class EditMulti3 {
				control = "EditMulti3";
				defaultValue = "''";
				expression = "_this setVariable ['%s', _value, true];";
			};
			class EditMulti5 {
				control = "EditMulti5";
				defaultValue = "''";
				expression = "_this setVariable ['%s', _value, true];";
			};
			class ModuleDescription;
			class Units;
		};
		class ModuleDescription 
		{
			class AnyBrain;
			class AnyPerson;
			class AnyPlayer;
			class AnyStaticObject;
			class Anything;
			class AnyVehicle;
			class EmptyDetector;
		};
	};
	class Static;
	class Land_InvisibleBarrier_F;
	class KH_HelperArrow: Static
	{
		author = "Kharos";
		displayName = "Helper (Arrow)";
		editorCategory = "KH_Helpers";
		editorSubcategory = "KH_Invisible";
		vehicleClass = "KH_Helpers";
		icon = "\x\kh\addons\main\ui\helper_arrow_up.paa";
		scope = 2;
	};
	class KH_HelperSquare: Static
	{
		author = "Kharos";
		displayName = "Helper (Square)";
		editorCategory = "KH_Helpers";
		editorSubcategory = "KH_Invisible";
		icon = "iconObject_1x1";
		vehicleClass = "KH_Helpers";
		scope = 2;
	};
	class KH_HelperRectangle_1x1x2: Static
	{
		author = "Kharos";
		displayName = "Helper Rectangle 1x1x2";
		editorCategory = "KH_Helpers";
		editorSubcategory = "KH_Invisible";
		model = "\x\kh\addons\main\models\SM_HelperRectangle_1x1x2.p3d";
		vehicleClass = "KH_Helpers";
		scope = 2;
	};
	class KH_InvisibleWall_1x1x1: Land_InvisibleBarrier_F
	{
		author = "Kharos";
		displayName = "Invisible Wall 1x1x1";
		editorCategory = "KH_Helpers";
		editorSubcategory = "KH_Invisible";
		model = "\x\kh\addons\main\models\SM_InvisibleWall_1x1x1.p3d";
		vehicleClass = "KH_Helpers";
		scope = 2;
	};
	class KH_InvisibleWall_2x1x1: Land_InvisibleBarrier_F
	{
		author = "Kharos";
		displayName = "Invisible Wall 2x1x1";
		editorCategory = "KH_Helpers";
		editorSubcategory = "KH_Invisible";
		model = "\x\kh\addons\main\models\SM_InvisibleWall_2x1x1.p3d";
		vehicleClass = "KH_Helpers";
		scope = 2;
	};
	class KH_InvisibleWall_2x1x2: Land_InvisibleBarrier_F
	{
		author = "Kharos";
		displayName = "Invisible Wall 2x1x2";
		editorCategory = "KH_Helpers";
		editorSubcategory = "KH_Invisible";
		model = "\x\kh\addons\main\models\SM_InvisibleWall_2x1x2.p3d";
		vehicleClass = "KH_Helpers";
		scope = 2;
	};
	class KH_InvisibleWall_4x1x2: Land_InvisibleBarrier_F
	{
		author = "Kharos";
		displayName = "Invisible Wall 4x1x2";
		editorCategory = "KH_Helpers";
		editorSubcategory = "KH_Invisible";
		model = "\x\kh\addons\main\models\SM_InvisibleWall_4x1x2.p3d";
		vehicleClass = "KH_Helpers";
		scope = 2;
	};
	class KH_InvisibleWall_4x1x4: Land_InvisibleBarrier_F
	{
		author = "Kharos";
		displayName = "Invisible Wall 4x1x4";
		editorCategory = "KH_Helpers";
		editorSubcategory = "KH_Invisible";
		model = "\x\kh\addons\main\models\SM_InvisibleWall_4x1x4.p3d";
		vehicleClass = "KH_Helpers";
		scope = 2;
	};
	class KH_InvisibleWall_8x1x4: Land_InvisibleBarrier_F
	{
		author = "Kharos";
		displayName = "Invisible Wall 8x1x4";
		editorCategory = "KH_Helpers";
		editorSubcategory = "KH_Invisible";
		model = "\x\kh\addons\main\models\SM_InvisibleWall_8x1x4.p3d";
		vehicleClass = "KH_Helpers";
		scope = 2;
	};
	class KH_InvisibleWall_8x1x8: Land_InvisibleBarrier_F
	{
		author = "Kharos";
		displayName = "Invisible Wall 8x1x8";
		editorCategory = "KH_Helpers";
		editorSubcategory = "KH_Invisible";
		model = "\x\kh\addons\main\models\SM_InvisibleWall_8x1x8.p3d";
		vehicleClass = "KH_Helpers";
		scope = 2;
	};
	class KH_InvisibleWall_16x1x8: Land_InvisibleBarrier_F
	{
		author = "Kharos";
		displayName = "Invisible Wall 16x1x8";
		editorCategory = "KH_Helpers";
		editorSubcategory = "KH_Invisible";
		model = "\x\kh\addons\main\models\SM_InvisibleWall_16x1x8.p3d";
		vehicleClass = "KH_Helpers";
		scope = 2;
	};
	class KH_InvisibleWall_16x1x16: Land_InvisibleBarrier_F
	{
		author = "Kharos";
		displayName = "Invisible Wall 16x1x16";
		editorCategory = "KH_Helpers";
		editorSubcategory = "KH_Invisible";
		model = "\x\kh\addons\main\models\SM_InvisibleWall_16x1x16.p3d";
		vehicleClass = "KH_Helpers";
		scope = 2;
	};
	class KH_InvisibleWall_32x1x16: Land_InvisibleBarrier_F
	{
		author = "Kharos";
		displayName = "Invisible Wall 32x1x16";
		editorCategory = "KH_Helpers";
		editorSubcategory = "KH_Invisible";
		model = "\x\kh\addons\main\models\SM_InvisibleWall_32x1x16.p3d";
		vehicleClass = "KH_Helpers";
		scope = 2;
	};
	class KH_InvisibleWall_32x1x32: Land_InvisibleBarrier_F
	{
		author = "Kharos";
		displayName = "Invisible Wall 32x1x32";
		editorCategory = "KH_Helpers";
		editorSubcategory = "KH_Invisible";
		model = "\x\kh\addons\main\models\SM_InvisibleWall_32x1x32.p3d";
		vehicleClass = "KH_Helpers";
		scope = 2;
	};
	#include "\x\kh\addons\main\vehicles\modules\addAction.hpp"
	#include "\x\kh\addons\main\vehicles\modules\assignIdentificationCard.hpp"
	#include "\x\kh\addons\main\vehicles\modules\assignTerminal.hpp"
	#include "\x\kh\addons\main\vehicles\modules\dynamicDisguise.hpp"
	#include "\x\kh\addons\main\vehicles\modules\dynamicDisguiseEquipment.hpp"
	#include "\x\kh\addons\main\vehicles\modules\endMission.hpp"
	#include "\x\kh\addons\main\vehicles\modules\entitySpawner.hpp"
	#include "\x\kh\addons\main\vehicles\modules\equipableObject.hpp"
	#include "\x\kh\addons\main\vehicles\modules\fultonExtract.hpp"
	#include "\x\kh\addons\main\vehicles\modules\teleporter.hpp"
	#include "\x\kh\addons\main\vehicles\modules\visualTrigger.hpp"
	#include "\x\kh\addons\main\vehicles\modules\zeusArma.hpp"
	#include "\x\kh\addons\main\vehicles\modules\zeusBreakDisguise.hpp"
	#include "\x\kh\addons\main\vehicles\modules\zeusEndMission.hpp"
	#include "\x\kh\addons\main\vehicles\modules\zeusHeal.hpp"
	#include "\x\kh\addons\main\vehicles\modules\zeusHealAllPlayers.hpp"
	#include "\x\kh\addons\main\vehicles\modules\zeusKill.hpp"
	#include "\x\kh\addons\main\vehicles\modules\zeusRagdollUnit.hpp"
	#include "\x\kh\addons\main\vehicles\modules\zeusToggleMissionSuspension.hpp"
};