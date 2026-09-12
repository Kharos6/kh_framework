class Cfg3DEN
{
	class EventHandlers
	{
		class KH_Framework
		{
			onEditableEntityAdded = "call KH_fnc_entityRenderingSetup;";
			onEditableEntityRemoved = "if ((param [0]) isEqualType objNull) then {{{removeRenderHandler _x;} forEach _y;} forEach ((param [0]) getVariable ['KH_var_renderHandlers', createHashMap]);};";
			onMissionPreviewEnd = "[] spawn {sleep 1; isNil {{[_x] call KH_fnc_entityRenderingSetup} forEach (allMissionObjects '');};};";
		};
	};
	#include "\x\kh\addons\main\3den\attributes.hpp"
	#include "\x\kh\addons\main\3den\mission.hpp"
	#include "\x\kh\addons\main\3den\object.hpp"
	#include "\x\kh\addons\main\3den\group.hpp"
};