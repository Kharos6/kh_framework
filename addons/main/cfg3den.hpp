class Cfg3DEN
{
	class EventHandlers
	{
		class KH_Framework
		{
			onTerrainNew = "removeAllRenderHandlers;";
			onEditableEntityAdded = "if (!(((param [0]) get3DENAttribute 'ControlSP') select 0) && !(((param [0]) get3DENAttribute 'ControlMP') select 0)) then {(param [0]) set3DENAttribute ['KH_EdenID', str (get3DENEntityID (param [0]))];}; call KH_fnc_entityRenderingSetup;";
			onEditableEntityRemoved = "if ((param [0]) isEqualType objNull) then {{{removeRenderHandler (_x select 0);} forEach _y;} forEach ((param [0]) getVariable ['KH_var_renderHandlers', createHashMap]);};";
			onMissionPreviewEnd = "[] spawn {sleep 1; isNil {{[_x] call KH_fnc_entityRenderingSetup;} forEach (allMissionObjects '');};};";
			onMissionLoad = "[] spawn {sleep 1; isNil {{if (((_x get3DENAttribute 'ControlSP') select 0) || ((_x get3DENAttribute 'ControlMP') select 0)) then {continue;}; _x set3DENAttribute ['KH_EdenID', str (get3DENEntityID _x)];} forEach ((all3DENEntities) select 0); {[_x] call KH_fnc_entityRenderingSetup;} forEach (allMissionObjects '');};};";
			onMissionSave = "{if (((_x get3DENAttribute 'ControlSP') select 0) || ((_x get3DENAttribute 'ControlMP') select 0)) then {continue;}; _x set3DENAttribute ['KH_EdenID', str (get3DENEntityID _x)];} forEach ((all3DENEntities) select 0);";
			onMissionAutosave = "{if (((_x get3DENAttribute 'ControlSP') select 0) || ((_x get3DENAttribute 'ControlMP') select 0)) then {continue;}; _x set3DENAttribute ['KH_EdenID', str (get3DENEntityID _x)];} forEach ((all3DENEntities) select 0);";
			onMissionExportSP = "{if (((_x get3DENAttribute 'ControlSP') select 0) || ((_x get3DENAttribute 'ControlMP') select 0)) then {continue;}; _x set3DENAttribute ['KH_EdenID', str (get3DENEntityID _x)];} forEach ((all3DENEntities) select 0);";
			onMissionExportMP = "{if (((_x get3DENAttribute 'ControlSP') select 0) || ((_x get3DENAttribute 'ControlMP') select 0)) then {continue;}; _x set3DENAttribute ['KH_EdenID', str (get3DENEntityID _x)];} forEach ((all3DENEntities) select 0);";
		};
	};
	#include "\x\kh\addons\main\3den\attributes.hpp"
	#include "\x\kh\addons\main\3den\mission.hpp"
	#include "\x\kh\addons\main\3den\object.hpp"
	#include "\x\kh\addons\main\3den\group.hpp"
};