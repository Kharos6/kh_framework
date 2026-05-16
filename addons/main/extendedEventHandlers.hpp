class Extended_PreInit_EventHandlers
{
    class KH_Main
	{
		init = "call (compileScript ['\x\kh\addons\main\functions\XEH_preInit.sqf'])";
    };
};
class Extended_Init_Eventhandlers
{
    class All
    {
        class KH_Main
        {
            init = "[param [0], false] call KH_fnc_entityInitHandler";
        };
    };
};
class Extended_Respawn_Eventhandlers
{
    class All
    {
        class KH_Main
        {
            respawn = "[param [0], true] call KH_fnc_entityInitHandler";
        };
    };
};
class Extended_Deleted_Eventhandlers
{
    class All
    {
        class KH_Main
        {
            deleted = "call KH_fnc_entityDeletedHandler";
        };
    };
};