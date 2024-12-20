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
            text = "SUSPEND MISSION";
            x = QUOTE(RSC_POS_X(14.5));
            y = QUOTE(RSC_POS_Y(9.5));
            w = QUOTE(RSC_POS_W(11));
            h = QUOTE(RSC_POS_H(1));
            colorBackground[] = {GUI_THEME_RGB_R, GUI_THEME_RGB_G, GUI_THEME_RGB_B, GUI_THEME_ALPHA};
            moving = 1;
        };
        class KH_Background: RscText 
        {
            x = QUOTE(RSC_POS_X(14.5));
            y = QUOTE(RSC_POS_Y(10.5));
            w = QUOTE(RSC_POS_W(11));
            h = QUOTE(RSC_POS_W(2));
            colorBackground[] = {0, 0, 0, 0.9};
        };
        class KH_SuspendTitle: RscText
        {
            text = "Suspend";
            x = QUOTE(RSC_POS_X(19));
            y = QUOTE(RSC_POS_Y(10.25));
            w = QUOTE(RSC_POS_W(6));
            h = QUOTE(RSC_POS_H(2));
            colorBackground[] = {0, 0, 0, 0};
        };
        class KH_Suspend: RscCheckbox
        {
            idc = 100;
            x = QUOTE(RSC_POS_X(18));
            y = QUOTE(RSC_POS_Y(10.75));
            w = QUOTE(RSC_POS_W(1));
            h = QUOTE(RSC_POS_W(1.25));
        };
        class KH_ButtonOK: RscButtonMenuOK 
        {
            x = QUOTE(RSC_POS_X(20.5));
            y = QUOTE(RSC_POS_Y(12));
            w = QUOTE(RSC_POS_W(5));
            h = QUOTE(RSC_POS_H(1));
        };
        class KH_ButtonCancel: RscButtonMenuCancel 
        {
            x = QUOTE(RSC_POS_X(14.5));
            y = QUOTE(RSC_POS_Y(12));
            w = QUOTE(RSC_POS_W(5));
            h = QUOTE(RSC_POS_H(1));
        };
    };
};