class KH_ResourceTerminal
{
    idd = -1;
    function = "";
    movingEnable = 1;
    onLoad = "";
    class Controls 
    {
        class Title: RscText
        {
            idc = 100;
            text = "";
            x = QUOTE(RSC_POS_X(20));
            y = QUOTE(RSC_POS_Y(18));
            w = QUOTE(RSC_POS_W(60));
            h = QUOTE(RSC_POS_H(2));
            colorBackground[] = {GUI_THEME_RGB_R, GUI_THEME_RGB_G, GUI_THEME_RGB_B, GUI_THEME_ALPHA};
            moving = 1;
        };
        class Information: RscEditMulti
        {
            idc = 101;
            text = "";
            font = "EtelkaMonospacePro";
            canModify = 0;
            x = QUOTE(RSC_POS_X(20));
            y = QUOTE(RSC_POS_Y(20));
            w = QUOTE(RSC_POS_W(60));
            h = QUOTE(RSC_POS_H(7.5));
            colorBackground[] = {0, 0, 0, 0.9};
        };
        class Console: RscText
        {
            text = "CONSOLE";
            x = QUOTE(RSC_POS_X(20));
            y = QUOTE(RSC_POS_Y(28));
            w = QUOTE(RSC_POS_W(60));
            h = QUOTE(RSC_POS_H(2));
            moving = 1;
            colorBackground[] = {GUI_THEME_RGB_R, GUI_THEME_RGB_G, GUI_THEME_RGB_B, GUI_THEME_ALPHA};
        };
        class Output: RscEditMulti
        {
            idc = 102;
            text = "";
            font = "EtelkaMonospacePro";
            canModify = 0;
            x = QUOTE(RSC_POS_X(20));
            y = QUOTE(RSC_POS_Y(30));
            w = QUOTE(RSC_POS_W(60));
            h = QUOTE(RSC_POS_H(30));
            colorBackground[] = {0, 0, 0, 0.9};
        };
        class Input: RscEditMulti
        {
            idc = 103;
            font = "EtelkaMonospacePro";
            x = QUOTE(RSC_POS_X(20));
            y = QUOTE(RSC_POS_Y(60.5));
            w = QUOTE(RSC_POS_W(60));
            h = QUOTE(RSC_POS_H(19.5));
            colorBackground[] = {0, 0, 0, 0.9};
        };
        class ButtonExecute: RscButton
        {
            idc = 104;
			text = "EXECUTE";
            x = QUOTE(RSC_POS_X(20));
            y = QUOTE(RSC_POS_Y(80.5));
            w = QUOTE(RSC_POS_W(10));
            h = QUOTE(RSC_POS_H(2));
            colorBackground[] = {0, 0, 0, 0.9};
            colorBackgroundActive[] = {1, 1, 1, 0.9};
        };
        class ButtonExit: RscButtonMenuCancel
        {
			text = "EXIT";
            x = QUOTE(RSC_POS_X(70));
            y = QUOTE(RSC_POS_Y(80.5));
            w = QUOTE(RSC_POS_W(10));
            h = QUOTE(RSC_POS_H(2));
            colorBackground[] = {0, 0, 0, 0.9};
            colorBackgroundActive[] = {1, 1, 1, 0.9};
        };
    };
};