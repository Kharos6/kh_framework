class KH_ResourceDeleteEmptyGroups
{
    idd = -1;
    function = "KH_fnc_zeusModuleDeleteEmptyGroups";
    movingEnable = 1;
    onLoad = "[_this select 0, 'KH_ResourceDeleteEmptyGroups'] call KH_fnc_loadDisplay";
    class Controls 
    {
        class KH_Title: RscText 
        {
            idc = 100;
            text = "DELETE EMPTY GROUPS";
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
            h = QUOTE(RSC_POS_H(2.5));
            colorBackground[] = {0, 0, 0, 0.9};
        };
        class KH_GlobalTitle: RscText
        {
			text = "Global";
            tooltip = "True deletes all groups, not just the local ones.";
            x = QUOTE(RSC_POS_X(40));
            y = QUOTE(RSC_POS_Y(40));
            w = QUOTE(RSC_POS_W(10));
            h = QUOTE(RSC_POS_H(2));
        };
        class KH_Global: ctrlCheckbox
        {
			idc = 101;
            x = QUOTE(RSC_POS_X(50));
            y = QUOTE(RSC_POS_Y(40));
            w = QUOTE(RSC_POS_W_SQUARE(1));
            h = QUOTE(RSC_POS_H_SQUARE(1));
        };
        class KH_ButtonOK: RscButtonMenuOK 
        {
            x = QUOTE(RSC_POS_X(55));
            y = QUOTE(RSC_POS_Y(43));
            w = QUOTE(RSC_POS_W(5));
            h = QUOTE(RSC_POS_H(2));
            colorBackground[] = {0, 0, 0, 0.9};
            colorBackgroundActive[] = {1, 1, 1, 0.9};
        };
        class KH_ButtonCancel: RscButtonMenuCancel 
        {
            x = QUOTE(RSC_POS_X(40));
            y = QUOTE(RSC_POS_Y(43));
            w = QUOTE(RSC_POS_W(5));
            h = QUOTE(RSC_POS_H(2));
            colorBackground[] = {0, 0, 0, 0.9};
            colorBackgroundActive[] = {1, 1, 1, 0.9};
        };
    };
};