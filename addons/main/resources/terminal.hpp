class KH_ResourceTerminal
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
            text = "";
            x = QUOTE(RSC_POS_X(6.5));
            y = QUOTE(RSC_POS_Y(0));
            w = QUOTE(RSC_POS_W(27));
            h = QUOTE(RSC_POS_H(1));
            colorBackground[] = {GUI_THEME_RGB_R, GUI_THEME_RGB_G, GUI_THEME_RGB_B, GUI_THEME_ALPHA};
            moving = 1;
        };
        class KH_Background: RscText 
        {
            x = QUOTE(RSC_POS_X(6.5));
            y = QUOTE(RSC_POS_Y(6));
            w = QUOTE(RSC_POS_W(27));
            h = QUOTE(RSC_POS_W(27));
            colorBackground[] = {0, 0, 0, 0.9};
        };
        class KH_Information: RscTextMulti
        {
            idc = 101;
            text = "";
            x = QUOTE(RSC_POS_X(6.5));
            y = QUOTE(RSC_POS_Y(1));
            w = QUOTE(RSC_POS_W(27));
            h = QUOTE(RSC_POS_W(5));
            colorBackground[] = {0, 0, 0, 0.9};
        };
        class KH_Console: RscText 
        {
            text = "CONSOLE";
            x = QUOTE(RSC_POS_X(6.5));
            y = QUOTE(RSC_POS_Y(4.75));
            w = QUOTE(RSC_POS_W(27));
            h = QUOTE(RSC_POS_H(1.25));
            colorBackground[] = {GUI_THEME_RGB_R, GUI_THEME_RGB_G, GUI_THEME_RGB_B, GUI_THEME_ALPHA};
        };
        class KH_Output: RscTextMulti
        {
            idc = 102;
            text = "";
            x = QUOTE(RSC_POS_X(6.5));
            y = QUOTE(RSC_POS_Y(6));
            w = QUOTE(RSC_POS_W(27));
            h = QUOTE(RSC_POS_W(27));
            colorBackground[] = {0, 0, 0, 0};
        };
        class KH_Input: RscEdit
        {
            idc = 103;
            font = "EtelkaMonospacePro";
            x = QUOTE(RSC_POS_X(6.5));
            y = QUOTE(RSC_POS_Y(26.15));
            w = QUOTE(RSC_POS_W(27));
            h = QUOTE(RSC_POS_W(1.5));
            colorBackground[] = {0, 0, 0, 0.9};
        };
        class KH_ButtonExecute: RscButton 
        {
            idc = 104;
			text = "EXECUTE";
            x = QUOTE(RSC_POS_X(28.5));
            y = QUOTE(RSC_POS_Y(27.25));
            w = QUOTE(RSC_POS_W(5));
            h = QUOTE(RSC_POS_H(1));
        };
        class KH_ButtonExit: RscButtonMenuCancel 
        {
			text = "EXIT";
            x = QUOTE(RSC_POS_X(6.5));
            y = QUOTE(RSC_POS_Y(27.25));
            w = QUOTE(RSC_POS_W(5));
            h = QUOTE(RSC_POS_H(1));
        };
    };
};