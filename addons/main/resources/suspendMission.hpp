class KH_ResourceSuspendMission
{
    idd = -1;
    function = "KH_fnc_zeusModuleSuspendMission";
    movingEnable = 1;
    onLoad = "[_this select 0, 'KH_ResourceSuspendMission'] call KH_fnc_loadDisplay";
    class Controls 
    {
        class KH_Title: RscText 
        {
            idc = 100;
            text = "SUSPEND MISSION";
            x = QUOTE(RSC_POS_X(40));
            y = QUOTE(RSC_POS_Y(38));
            w = QUOTE(RSC_POS_W(20));
            h = QUOTE(RSC_POS_H(2));
            colorBackground[] = {GUI_THEME_RGB_R, GUI_THEME_RGB_G, GUI_THEME_RGB_B, GUI_THEME_ALPHA};
            moving = 1;
        };
        class KH_Background: RscText 
        {
            x = QUOTE(RSC_POS_X(40));
            y = QUOTE(RSC_POS_Y(40));
            w = QUOTE(RSC_POS_W(20));
            h = QUOTE(RSC_POS_H(4));
            colorBackground[] = {0, 0, 0, 0.9};
        };
        class KH_SuspendTitle: RscText
        {
			text = "Suspend";
            tooltip = "True suspends the mission. False resumes it.";
            x = QUOTE(RSC_POS_X(40));
            y = QUOTE(RSC_POS_Y(40));
            w = QUOTE(RSC_POS_W(10));
            h = QUOTE(RSC_POS_W(2));
        };
        class KH_Suspend: ctrlCheckbox
        {
			idc = 101;
            x = QUOTE(RSC_POS_X(50));
            y = QUOTE(RSC_POS_Y(40));
            w = QUOTE(RSC_POS_W(2));
            h = QUOTE(RSC_POS_W(2));
        };
        class KH_ToggleSimulationTitle: RscText
        {
			text = "Toggle Simulation";
            tooltip = "By default, players and their vehicles have their simulation disabled; this option goes further and also disables all AI and their vehicles if true, also resuming it if they were previously suspended if true.";
            x = QUOTE(RSC_POS_X(40));
            y = QUOTE(RSC_POS_Y(42));
            w = QUOTE(RSC_POS_W(10));
            h = QUOTE(RSC_POS_W(2));
        };
        class KH_ToggleSimulation: ctrlCheckbox
        {
			idc = 102;
            x = QUOTE(RSC_POS_X(50));
            y = QUOTE(RSC_POS_Y(42));
            w = QUOTE(RSC_POS_W(2));
            h = QUOTE(RSC_POS_W(2));
        };
        class KH_SuspensionTextTitle: RscText
        {
			text = "Suspension Text";
            tooltip = "Text displayed to players while the mission is suspended.";
            x = QUOTE(RSC_POS_X(40));
            y = QUOTE(RSC_POS_Y(44));
            w = QUOTE(RSC_POS_W(10));
            h = QUOTE(RSC_POS_W(2));
        };
        class KH_SuspensionText: ctrlEdit
        {
			idc = 103;
            text = "MISSION SUSPENDED - PLEASE WAIT";
            x = QUOTE(RSC_POS_X(50));
            y = QUOTE(RSC_POS_Y(44));
            w = QUOTE(RSC_POS_W(1));
            h = QUOTE(RSC_POS_W(1));
        };
        class KH_ButtonOK: RscButtonMenuOK 
        {
            x = QUOTE(RSC_POS_X(40));
            y = QUOTE(RSC_POS_Y(46.5));
            w = QUOTE(RSC_POS_W(5));
            h = QUOTE(RSC_POS_W(2));
            colorBackground[] = {0, 0, 0, 0.9};
            colorBackgroundActive[] = {1, 1, 1, 0.9};
        };
        class KH_ButtonCancel: RscButtonMenuCancel 
        {
            x = QUOTE(RSC_POS_X(55));
            y = QUOTE(RSC_POS_Y(46.5));
            w = QUOTE(RSC_POS_W(5));
            h = QUOTE(RSC_POS_W(2));
            colorBackground[] = {0, 0, 0, 0.9};
            colorBackgroundActive[] = {1, 1, 1, 0.9};
        };
    };
};