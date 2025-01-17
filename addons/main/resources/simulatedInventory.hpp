class KH_ResourceSimulatedInventory
{
    idd = -1;
    function = "";
    movingEnable = 1;
    onLoad = "";
    class Controls 
    {
        class KH_Title: RscText 
        {
			Idc = 101;
            text = "INVENTORY";
            x = QUOTE(RSC_POS_X(9.5));
            y = QUOTE(RSC_POS_Y(0));
            w = QUOTE(RSC_POS_W(21));
            h = QUOTE(RSC_POS_H(1));
            colorBackground[] = {GUI_THEME_RGB_R, GUI_THEME_RGB_G, GUI_THEME_RGB_B, GUI_THEME_ALPHA};
            moving = 1;
        };
        class KH_Background: RscText 
        {
            x = QUOTE(RSC_POS_X(9.5));
            y = QUOTE(RSC_POS_Y(1));
            w = QUOTE(RSC_POS_W(21));
            h = QUOTE(RSC_POS_W(30));
            colorBackground[] = {0, 0, 0, 0.9};
        };
        class KH_List: ctrlListbox
        {
            idc = 100;
            x = QUOTE(RSC_POS_X(9.5));
            y = QUOTE(RSC_POS_Y(1));
            w = QUOTE(RSC_POS_W(21));
            h = QUOTE(RSC_POS_W(30));
        };
        class KH_ButtonClose: RscButtonMenuCancel 
        {
			text = "CLOSE";
            x = QUOTE(RSC_POS_X(9.5));
            y = QUOTE(RSC_POS_Y(23.5));
            w = QUOTE(RSC_POS_W(5));
            h = QUOTE(RSC_POS_H(1));
        };
    };
};