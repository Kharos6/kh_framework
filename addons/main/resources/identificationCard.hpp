class KH_ResourceIdentificationCard
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
            text = "IDENTIFICATION CARD";
            x = QUOTE(RSC_POS_X(9.5));
            y = QUOTE(RSC_POS_Y(5));
            w = QUOTE(RSC_POS_W(21));
            h = QUOTE(RSC_POS_H(1));
            colorBackground[] = {GUI_THEME_RGB_R, GUI_THEME_RGB_G, GUI_THEME_RGB_B, GUI_THEME_ALPHA};
            moving = 1;
        };
        class KH_Background: RscText 
        {
            x = QUOTE(RSC_POS_X(9.5));
            y = QUOTE(RSC_POS_Y(6));
            w = QUOTE(RSC_POS_W(21));
            h = QUOTE(RSC_POS_W(12));
            colorBackground[] = {0, 0, 0, 0.9};
        };
        class KH_Name: RscText
        {
            idc = 101;
            x = QUOTE(RSC_POS_X(9.5));
            y = QUOTE(RSC_POS_Y(6));
            w = QUOTE(RSC_POS_W(21));
            h = QUOTE(RSC_POS_W(1));
        };
        class KH_Gender: RscText
        {
            idc = 102;
            x = QUOTE(RSC_POS_X(9.5));
            y = QUOTE(RSC_POS_Y(7));
            w = QUOTE(RSC_POS_W(21));
            h = QUOTE(RSC_POS_W(1));
        };
        class KH_Race: RscText
        {
            idc = 103;
            x = QUOTE(RSC_POS_X(9.5));
            y = QUOTE(RSC_POS_Y(8));
            w = QUOTE(RSC_POS_W(21));
            h = QUOTE(RSC_POS_W(1));
        };
        class KH_DateOfBirth: RscText
        {
            idc = 104;
            x = QUOTE(RSC_POS_X(9.5));
            y = QUOTE(RSC_POS_Y(9));
            w = QUOTE(RSC_POS_W(21));
            h = QUOTE(RSC_POS_W(1));
        };
        class KH_Profession: RscText
        {
            idc = 105;
            x = QUOTE(RSC_POS_X(9.5));
            y = QUOTE(RSC_POS_Y(10));
            w = QUOTE(RSC_POS_W(21));
            h = QUOTE(RSC_POS_W(1));
        };
        class KH_Ethnos: RscText
        {
            idc = 106;
            x = QUOTE(RSC_POS_X(9.5));
            y = QUOTE(RSC_POS_Y(11));
            w = QUOTE(RSC_POS_W(21));
            h = QUOTE(RSC_POS_W(1));
        };
        class KH_CardNumber: RscText
        {
            idc = 107;
            x = QUOTE(RSC_POS_X(9.5));
            y = QUOTE(RSC_POS_Y(12));
            w = QUOTE(RSC_POS_W(21));
            h = QUOTE(RSC_POS_W(1));
        };
        class KH_DateOfIssue: RscText
        {
            idc = 108;
            x = QUOTE(RSC_POS_X(9.5));
            y = QUOTE(RSC_POS_Y(13));
            w = QUOTE(RSC_POS_W(21));
            h = QUOTE(RSC_POS_W(1));
        };
        class KH_DateOfExpiry: RscText
        {
            idc = 109;
            x = QUOTE(RSC_POS_X(9.5));
            y = QUOTE(RSC_POS_Y(14));
            w = QUOTE(RSC_POS_W(21));
            h = QUOTE(RSC_POS_W(1));
        };
        class KH_ButtonClose: RscButtonMenuCancel 
        {
			text = "CLOSE";
            x = QUOTE(RSC_POS_X(9.5));
            y = QUOTE(RSC_POS_Y(15));
            w = QUOTE(RSC_POS_W(5));
            h = QUOTE(RSC_POS_H(1));
        };
    };
};