class KH_ResourceRemoteController
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
            text = "REMOTE CONTROLLER";
            x = QUOTE(RSC_POS_X(40));
            y = QUOTE(RSC_POS_Y(28));
            w = QUOTE(RSC_POS_W(20));
            h = QUOTE(RSC_POS_H(2));
            colorBackground[] = {GUI_THEME_RGB_R, GUI_THEME_RGB_G, GUI_THEME_RGB_B, GUI_THEME_ALPHA};
            moving = 1;
        };
        class UnitList: RscListBox
        {
            idc = 101;
            x = QUOTE(RSC_POS_X(40));
            y = QUOTE(RSC_POS_Y(30));
            w = QUOTE(RSC_POS_W(20));
            h = QUOTE(RSC_POS_H(30));
            colorBackground[] = {0, 0, 0, 0.9};
        };
        class UnitImage: KH_ResourcePicture
        {
            idc = 103;
            x = QUOTE(RSC_POS_X(60.5));
            y = QUOTE(RSC_POS_Y(30));
            w = QUOTE(RSC_POS_W(10));
            h = QUOTE(RSC_POS_H_SQUARE(10));
            colorBackground[] = {0, 0, 0, 0.9};
        };
        class UnitMap: RscMapControl
        {
            idc = 104;
            x = QUOTE(RSC_POS_X(29.5));
            y = QUOTE(RSC_POS_Y(30));
            w = QUOTE(RSC_POS_W(10));
            h = QUOTE(RSC_POS_H_SQUARE(10));
            colorBackground[] = {0, 0, 0, 0.9};
        };
        class ButtonClose: RscButtonMenuCancel 
        {
			text = "CLOSE";
            x = QUOTE(RSC_POS_X(50.5));
            y = QUOTE(RSC_POS_Y(60.5));
            w = QUOTE(RSC_POS_W(9.5));
            h = QUOTE(RSC_POS_H(2));
            colorBackground[] = {0, 0, 0, 0.9};
            colorBackgroundActive[] = {1, 1, 1, 0.9};
        };
        class ButtonControl: RscButton
        {
            idc = 102;
			text = "CONTROL";
            x = QUOTE(RSC_POS_X(40));
            y = QUOTE(RSC_POS_Y(60.5));
            w = QUOTE(RSC_POS_W(9.5));
            h = QUOTE(RSC_POS_H(2));
            colorBackground[] = {0, 0, 0, 0.9};
            colorBackgroundActive[] = {1, 1, 1, 0.9};
        };
    };
};