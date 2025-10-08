class KH_DebugConsole
{
    idd = -1;
    function = "";
    movingEnable = 1;
    onLoad = "";
    class Controls 
    {
        class KH_Title: RscText 
        {
            idc = 100;
            text = "DEBUG CONSOLE";
            x = QUOTE(RSC_POS_X(25));
            y = QUOTE(RSC_POS_Y(18));
            w = QUOTE(RSC_POS_W(50));
            h = QUOTE(RSC_POS_H(2));
            colorBackground[] = {GUI_THEME_RGB_R, GUI_THEME_RGB_G, GUI_THEME_RGB_B, GUI_THEME_ALPHA};
            moving = 1;
        };
        class KH_Input: RscEditMulti
        {
            idc = 101;
            font = "EtelkaMonospacePro";
            x = QUOTE(RSC_POS_X(25));
            y = QUOTE(RSC_POS_Y(20));
            w = QUOTE(RSC_POS_W(50));
            h = QUOTE(RSC_POS_H(40));
            colorBackground[] = {0, 0, 0, 0.9};
        };
        class KH_OutputTitle: RscText 
        {
            idc = 102;
            text = "OUTPUT";
            x = QUOTE(RSC_POS_X(25));
            y = QUOTE(RSC_POS_Y(60.5));
            w = QUOTE(RSC_POS_W(50));
            h = QUOTE(RSC_POS_H(2));
            colorBackground[] = {GUI_THEME_RGB_R, GUI_THEME_RGB_G, GUI_THEME_RGB_B, GUI_THEME_ALPHA};
            moving = 1;
        };
        class KH_Output: RscTextMulti
        {
            idc = 103;
            text = "";
            x = QUOTE(RSC_POS_X(25));
            y = QUOTE(RSC_POS_Y(62.5));
            w = QUOTE(RSC_POS_W(50));
            h = QUOTE(RSC_POS_H(10));
            colorBackground[] = {0, 0, 0, 0.9};
        };
        class KH_ButtonExecute: RscButton 
        {
            idc = 104;
			text = "EXECUTE";
            x = QUOTE(RSC_POS_X(25));
            y = QUOTE(RSC_POS_Y(73));
            w = QUOTE(RSC_POS_W(24.5));
            h = QUOTE(RSC_POS_H(2));
            colorBackground[] = {0, 0, 0, 0.9};
            colorBackgroundActive[] = {1, 1, 1, 0.9};
        };
        class KH_ButtonProfile: RscButton 
        {
            idc = 105;
			text = "PROFILE";
            x = QUOTE(RSC_POS_X(50.5));
            y = QUOTE(RSC_POS_Y(73));
            w = QUOTE(RSC_POS_W(24.5));
            h = QUOTE(RSC_POS_H(2));
            colorBackground[] = {0, 0, 0, 0.9};
            colorBackgroundActive[] = {1, 1, 1, 0.9};
        };
        class KH_ButtonPrevious: RscButton 
        {
            idc = 106;
			text = "PREVIOUS";
            x = QUOTE(RSC_POS_X(25));
            y = QUOTE(RSC_POS_Y(75.5));
            w = QUOTE(RSC_POS_W(24.5));
            h = QUOTE(RSC_POS_H(2));
            colorBackground[] = {0, 0, 0, 0.9};
            colorBackgroundActive[] = {1, 1, 1, 0.9};
        };
        class KH_ButtonNext: RscButton 
        {
            idc = 107;
			text = "NEXT";
            x = QUOTE(RSC_POS_X(50.5));
            y = QUOTE(RSC_POS_Y(75.5));
            w = QUOTE(RSC_POS_W(24.5));
            h = QUOTE(RSC_POS_H(2));
            colorBackground[] = {0, 0, 0, 0.9};
            colorBackgroundActive[] = {1, 1, 1, 0.9};
        };
        class KH_Language: RscToolbox 
        {
            idc = 108;
            x = QUOTE(RSC_POS_X(25));
            y = QUOTE(RSC_POS_Y(15));
            w = QUOTE(RSC_POS_W(50));
            h = QUOTE(RSC_POS_H(2));
            colorSelectedBg[] = {GUI_THEME_RGB_R, GUI_THEME_RGB_G, GUI_THEME_RGB_B, GUI_THEME_ALPHA};
            colorBackground[] = {0, 0, 0, 0.9};
            columns = 2;
            rows = 1;
            strings[] ={"SQF", "LUA"};
        };
        class KH_ButtonExit: RscButtonMenuCancel 
        {
			text = "EXIT";
            x = QUOTE(RSC_POS_X(25));
            y = QUOTE(RSC_POS_Y(78));
            w = QUOTE(RSC_POS_W(50));
            h = QUOTE(RSC_POS_H(2));
            colorBackground[] = {0, 0, 0, 0.9};
            colorBackgroundActive[] = {1, 1, 1, 0.9};
        };
    };
};