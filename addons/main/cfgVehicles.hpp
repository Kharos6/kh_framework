class CfgVehicles
{
	class Module_F
	{
		class AttributesBase
		{
			class Checkbox;
			class CheckboxNumber;
			class Combo;
			class Default;
			class Edit;
			class ModuleDescription;
			class Units;
		};
		class ModuleDescription;
	};
	class Land_HelipadEmpty_F;
	class KH_HelperArrow: Land_HelipadEmpty_F
	{
		displayName = "Helper (Arrow)";
		editorCategory = "KH_Helpers";
		editorSubcategory = "KH_Invisible";
		vehicleClass = "KH_Helpers";
		icon = "\x\kh\addons\main\ui\helper_arrow_up.paa";
	};	
	class KH_HelperSquare: Land_HelipadEmpty_F
	{
		displayName = "Helper (Square)";
		editorCategory = "KH_Helpers";
		editorSubcategory = "KH_Invisible";
		vehicleClass = "KH_Helpers";
	};
	#include "\x\kh\addons\main\vehicles\modules\addInteractionEntry.hpp"
	#include "\x\kh\addons\main\vehicles\modules\assignParachute.hpp"
	#include "\x\kh\addons\main\vehicles\modules\breakDisguise.hpp"
	#include "\x\kh\addons\main\vehicles\modules\cameraSequence.hpp"
	#include "\x\kh\addons\main\vehicles\modules\displaySubtitle.hpp"
	#include "\x\kh\addons\main\vehicles\modules\dynamicDisguiseEquipment.hpp"
	#include "\x\kh\addons\main\vehicles\modules\editTerrainObjects.hpp"
	#include "\x\kh\addons\main\vehicles\modules\endMission.hpp"
	#include "\x\kh\addons\main\vehicles\modules\entitySpawner.hpp"
	#include "\x\kh\addons\main\vehicles\modules\manageSimulatedInventory.hpp"
	#include "\x\kh\addons\main\vehicles\modules\playMusic.hpp"
	#include "\x\kh\addons\main\vehicles\modules\playSound2d.hpp"
	#include "\x\kh\addons\main\vehicles\modules\playSound3d.hpp"
	#include "\x\kh\addons\main\vehicles\modules\setAnimation.hpp"
	#include "\x\kh\addons\main\vehicles\modules\suspendMission.hpp"
	#include "\x\kh\addons\main\vehicles\modules\teleport.hpp"
	#include "\x\kh\addons\main\vehicles\modules\vehicleSequence.hpp"
	#include "\x\kh\addons\main\vehicles\modules\zeusArma.hpp"
	#include "\x\kh\addons\main\vehicles\modules\zeusBreakDisguise.hpp"
	#include "\x\kh\addons\main\vehicles\modules\zeusClearStagedExecutions.hpp"
	#include "\x\kh\addons\main\vehicles\modules\zeusEndMission.hpp"
	#include "\x\kh\addons\main\vehicles\modules\zeusRagdollUnit.hpp"
	#include "\x\kh\addons\main\vehicles\modules\zeusStageExecution.hpp"
	#include "\x\kh\addons\main\vehicles\modules\zeusSuspendMission.hpp"
};