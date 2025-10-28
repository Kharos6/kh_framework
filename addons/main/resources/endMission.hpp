class KH_ResourceEndMission
{
    idd = -1;
    function = "KH_fnc_zeusModuleEndMission";
    movingEnable = 1;
    onLoad = "[_this select 0, 'KH_ResourceEndMission'] call KH_fnc_loadDisplay";
    class Controls 
    {
        class KH_Title: RscText 
        {
            idc = 100;
            text = "END MISSION";
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
        class KH_VictoryTitle: RscText
        {
			text = "Victory";
            tooltip = "True considers the end a victory.";
            x = QUOTE(RSC_POS_X(40));
            y = QUOTE(RSC_POS_Y(40));
            w = QUOTE(RSC_POS_W(10));
            h = QUOTE(RSC_POS_W(2));
        };
        class KH_Victory: ctrlCheckbox
        {
			idc = 101;
            x = QUOTE(RSC_POS_X(50));
            y = QUOTE(RSC_POS_Y(40));
            w = QUOTE(RSC_POS_W(1));
            h = QUOTE(RSC_POS_W(1));
        };
        class KH_FadeTypeTitle: RscText
        {
			text = "Fade Type";
            tooltip = "Type of fade to play. -2 instantly ends the mission without the signature closing shot and music, -1 plays the signature closing shot and music, and any value of 0 and above is a fade to black for that amount of seconds.";
            x = QUOTE(RSC_POS_X(40));
            y = QUOTE(RSC_POS_Y(42));
            w = QUOTE(RSC_POS_W(10));
            h = QUOTE(RSC_POS_W(2));
        };
        class KH_FadeType: ctrlEdit
        {
			idc = 102;
            text = "-1";
            x = QUOTE(RSC_POS_X(50));
            y = QUOTE(RSC_POS_Y(42));
            w = QUOTE(RSC_POS_W(1));
            h = QUOTE(RSC_POS_W(1));
        };
        class KH_ButtonOK: RscButtonMenuOK 
        {
            x = QUOTE(RSC_POS_X(40));
            y = QUOTE(RSC_POS_Y(44.5));
            w = QUOTE(RSC_POS_W(5));
            h = QUOTE(RSC_POS_W(2));
            colorBackground[] = {0, 0, 0, 0.9};
            colorBackgroundActive[] = {1, 1, 1, 0.9};
        };
        class KH_ButtonCancel: RscButtonMenuCancel 
        {
            x = QUOTE(RSC_POS_X(55));
            y = QUOTE(RSC_POS_Y(44.5));
            w = QUOTE(RSC_POS_W(5));
            h = QUOTE(RSC_POS_W(2));
            colorBackground[] = {0, 0, 0, 0.9};
            colorBackgroundActive[] = {1, 1, 1, 0.9};
        };
    };
};