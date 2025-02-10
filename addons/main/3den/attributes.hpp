class Attributes
{
	class Default;
	class Title: Default
	{
		class Controls
		{
			class Title;
		};
	};
	class SubCategoryNoHeader1: Default
	{
		class Controls
		{
			class Group: ctrlControlsGroup
			{
				class Controls
				{
					class Title;
					class DescriptionDeco;
					class Description;
				};
			};
		};
	};
	class KH_SubcategoryNoHeader1: SubCategoryNoHeader1
	{
		attributeLoad = "((_this controlsGroupCtrl 101) controlsGroupCtrl 102) ctrlSetStructuredText (parseText (getText (_config >> 'description')))";
		attributeSave = "";
		h = QUOTE(GRID_H * 6);
		class Controls: Controls
		{
			class Group: Group
			{
				h = QUOTE(4.5 * GRID_H);
				class Controls: Controls
				{
					class Title: Title
					{
						h = QUOTE(4.5 * GRID_H);
					};
					class DescriptionDeco: DescriptionDeco
					{
						h = QUOTE(4.5 * GRID_H);
					};
					class Description: Description
					{
						h = QUOTE(4.5 * GRID_H);
					};
				};
			};
		};
	};
	class KH_SubcategoryNoHeader2: SubCategoryNoHeader1
	{
		attributeLoad = "((_this controlsGroupCtrl 101) controlsGroupCtrl 102) ctrlSetStructuredText (parseText (getText (_config >> 'description')))";
		attributeSave = "";
		h = QUOTE((GRID_H * 6) * 2);
		class Controls: Controls
		{
			class Group: Group
			{
				h = QUOTE((4.5 * GRID_H) * 2);
				class Controls: Controls
				{
					class Title: Title
					{
						h = QUOTE((4.5 * GRID_H) * 2);
					};
					class DescriptionDeco: DescriptionDeco
					{
						h = QUOTE((4.5 * GRID_H) * 2);
					};
					class Description: Description
					{
						h = QUOTE((4.5 * GRID_H) * 2);
					};
				};
			};
		};
	};
	class KH_SubcategoryNoHeader3: SubCategoryNoHeader1
	{
		attributeLoad = "((_this controlsGroupCtrl 101) controlsGroupCtrl 102) ctrlSetStructuredText (parseText (getText (_config >> 'description')))";
		attributeSave = "";
		h = QUOTE((GRID_H * 6) * 3);
		class Controls: Controls
		{
			class Group: Group
			{
				h = QUOTE((4.5 * GRID_H) * 3);
				class Controls: Controls
				{
					class Title: Title
					{
						h = QUOTE((4.5 * GRID_H) * 3);
					};
					class DescriptionDeco: DescriptionDeco
					{
						h = QUOTE((4.5 * GRID_H) * 3);
					};
					class Description: Description
					{
						h = QUOTE((4.5 * GRID_H) * 3);
					};
				};
			};
		};
	};
	class KH_SubcategoryNoHeader4: SubCategoryNoHeader1
	{
		attributeLoad = "((_this controlsGroupCtrl 101) controlsGroupCtrl 102) ctrlSetStructuredText (parseText (getText (_config >> 'description')))";
		attributeSave = "";
		h = QUOTE((GRID_H * 6) * 4);
		class Controls: Controls
		{
			class Group: Group
			{
				h = QUOTE((4.5 * GRID_H) * 4);
				class Controls: Controls
				{
					class Title: Title
					{
						h = QUOTE((4.5 * GRID_H) * 4);
					};
					class DescriptionDeco: DescriptionDeco
					{
						h = QUOTE((4.5 * GRID_H) * 4);
					};
					class Description: Description
					{
						h = QUOTE((4.5 * GRID_H) * 4);
					};
				};
			};
		};
	};
	class KH_SubcategoryNoHeader5: SubCategoryNoHeader1
	{
		attributeLoad = "((_this controlsGroupCtrl 101) controlsGroupCtrl 102) ctrlSetStructuredText (parseText (getText (_config >> 'description')))";
		attributeSave = "";
		h = QUOTE((GRID_H * 6) * 5);
		class Controls: Controls
		{
			class Group: Group
			{
				h = QUOTE((4.5 * GRID_H) * 5);
				class Controls: Controls
				{
					class Title: Title
					{
						h = QUOTE((4.5 * GRID_H) * 5);
					};
					class DescriptionDeco: DescriptionDeco
					{
						h = QUOTE((4.5 * GRID_H) * 5);
					};
					class Description: Description
					{
						h = QUOTE((4.5 * GRID_H) * 5);
					};
				};
			};
		};
	};
	class KH_SubcategoryNoHeader6: SubCategoryNoHeader1
	{
		attributeLoad = "((_this controlsGroupCtrl 101) controlsGroupCtrl 102) ctrlSetStructuredText (parseText (getText (_config >> 'description')))";
		attributeSave = "";
		h = QUOTE((GRID_H * 6) * 6);
		class Controls: Controls
		{
			class Group: Group
			{
				h = QUOTE((4.5 * GRID_H) * 6);
				class Controls: Controls
				{
					class Title: Title
					{
						h = QUOTE((4.5 * GRID_H) * 6);
					};
					class DescriptionDeco: DescriptionDeco
					{
						h = QUOTE((4.5 * GRID_H) * 6);
					};
					class Description: Description
					{
						h = QUOTE((4.5 * GRID_H) * 6);
					};
				};
			};
		};
	};
	#include "\x\kh\addons\main\3den\at_assignIdentificationCard.hpp"
	#include "\x\kh\addons\main\3den\at_assignTerminal.hpp"
	#include "\x\kh\addons\main\3den\at_cameraSequence.hpp"
	#include "\x\kh\addons\main\3den\at_captureVehicleSequence.hpp"
	#include "\x\kh\addons\main\3den\at_conditionalPresence.hpp"
	#include "\x\kh\addons\main\3den\at_convertToAgent.hpp"
	#include "\x\kh\addons\main\3den\at_diagnostics.hpp"
	#include "\x\kh\addons\main\3den\at_dynamic2dAudio.hpp"
	#include "\x\kh\addons\main\3den\at_dynamic3dAudio.hpp"
	#include "\x\kh\addons\main\3den\at_dynamicDisguise.hpp"
	#include "\x\kh\addons\main\3den\at_environmentAmbiance.hpp"
	#include "\x\kh\addons\main\3den\at_equipableObject.hpp"
	#include "\x\kh\addons\main\3den\at_fultonExtract.hpp"
	#include "\x\kh\addons\main\3den\at_persistency.hpp"
	#include "\x\kh\addons\main\3den\at_persistentInit.hpp"
	#include "\x\kh\addons\main\3den\at_recoverDisconnectedPlayers.hpp"
	#include "\x\kh\addons\main\3den\at_respawnSpectator.hpp"
	#include "\x\kh\addons\main\3den\at_setAnimation.hpp"
	#include "\x\kh\addons\main\3den\at_setCameraTexture.hpp"
	#include "\x\kh\addons\main\3den\at_setCurators.hpp"
	#include "\x\kh\addons\main\3den\at_setSideRelations.hpp"
	#include "\x\kh\addons\main\3den\at_setVideoTexture.hpp"
	#include "\x\kh\addons\main\3den\at_teleporter.hpp"
	#include "\x\kh\addons\main\3den\at_transferToHeadlessClient.hpp"
	#include "\x\kh\addons\main\3den\at_unconsciousSpectator.hpp"
	#include "\x\kh\addons\main\3den\at_vehicleSequence.hpp"
	#include "\x\kh\addons\main\3den\at_visualTrigger.hpp"
};