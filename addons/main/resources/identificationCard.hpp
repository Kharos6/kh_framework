class KH_ResourceIdentificationCard
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
            text = "IDENTIFICATION CARD";
            x = QUOTE(RSC_POS_X(40));
            y = QUOTE(RSC_POS_Y(39));
            w = QUOTE(RSC_POS_W(20));
            h = QUOTE(RSC_POS_H(1));
            colorBackground[] = {GUI_THEME_RGB_R, GUI_THEME_RGB_G, GUI_THEME_RGB_B, GUI_THEME_ALPHA};
            moving = 1;
        };
        class Background: RscText 
        {
            x = QUOTE(RSC_POS_X(40));
            y = QUOTE(RSC_POS_Y(40));
            w = QUOTE(RSC_POS_W(20));
            h = QUOTE(RSC_POS_W(8));
            colorBackground[] = {0, 0, 0, 0.9};
        };
        class Name: RscText
        {
            idc = 101;
            x = QUOTE(RSC_POS_X(40));
            y = QUOTE(RSC_POS_Y(40));
            w = QUOTE(RSC_POS_W(20));
            h = QUOTE(RSC_POS_W(1));
        };
        class Gender: RscText
        {
            idc = 102;
            x = QUOTE(RSC_POS_X(40));
            y = QUOTE(RSC_POS_Y(41));
            w = QUOTE(RSC_POS_W(20));
            h = QUOTE(RSC_POS_W(1));
        };
        class Race: RscText
        {
            idc = 103;
            x = QUOTE(RSC_POS_X(40));
            y = QUOTE(RSC_POS_Y(42));
            w = QUOTE(RSC_POS_W(20));
            h = QUOTE(RSC_POS_W(1));
        };
        class DateOfBirth: RscText
        {
            idc = 104;
            x = QUOTE(RSC_POS_X(40));
            y = QUOTE(RSC_POS_Y(43));
            w = QUOTE(RSC_POS_W(20));
            h = QUOTE(RSC_POS_W(1));
        };
        class Profession: RscText
        {
            idc = 105;
            x = QUOTE(RSC_POS_X(40));
            y = QUOTE(RSC_POS_Y(44));
            w = QUOTE(RSC_POS_W(20));
            h = QUOTE(RSC_POS_W(1));
        };
        class Ethnos: RscText
        {
            idc = 106;
            x = QUOTE(RSC_POS_X(40));
            y = QUOTE(RSC_POS_Y(45));
            w = QUOTE(RSC_POS_W(20));
            h = QUOTE(RSC_POS_W(1));
        };
        class CardNumber: RscText
        {
            idc = 107;
            x = QUOTE(RSC_POS_X(40));
            y = QUOTE(RSC_POS_Y(46));
            w = QUOTE(RSC_POS_W(20));
            h = QUOTE(RSC_POS_W(1));
        };
        class DateOfIssue: RscText
        {
            idc = 108;
            x = QUOTE(RSC_POS_X(40));
            y = QUOTE(RSC_POS_Y(47));
            w = QUOTE(RSC_POS_W(20));
            h = QUOTE(RSC_POS_W(1));
        };
        class DateOfExpiry: RscText
        {
            idc = 109;
            x = QUOTE(RSC_POS_X(40));
            y = QUOTE(RSC_POS_Y(48));
            w = QUOTE(RSC_POS_W(20));
            h = QUOTE(RSC_POS_W(1));
        };
        class ButtonClose: RscButtonMenuCancel 
        {
			text = "CLOSE";
            x = QUOTE(RSC_POS_X(40));
            y = QUOTE(RSC_POS_Y(49.5));
            w = QUOTE(RSC_POS_W(20));
            h = QUOTE(RSC_POS_W(1));
        };
    };
};