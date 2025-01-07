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
            text = "END MISSION";
            tooltip = "Ends the mission, saving the mission state if that was enabled, and optionally starting the next mission.";
            x = QUOTE(RSC_POS_X(6.5));
            y = QUOTE(RSC_POS_Y(5));
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
            h = QUOTE(RSC_POS_W(7));
            colorBackground[] = {0, 0, 0, 0.9};
        };
        class KH_EndNameTitle: RscText
        {
			text = "End Name";
            tooltip = "End name from <configFile >> 'CfgDebriefing'>.";
            x = QUOTE(RSC_POS_X(6.5));
            y = QUOTE(RSC_POS_Y(6));
            w = QUOTE(RSC_POS_W(6));
            h = QUOTE(RSC_POS_W(1.25));
        };
        class KH_EndName: ctrlEdit
        {
			idc = 100;
            x = QUOTE(RSC_POS_X(11.5));
            y = QUOTE(RSC_POS_Y(6));
            w = QUOTE(RSC_POS_W(21));
            h = QUOTE(RSC_POS_W(1.25));
            colorBackground[] = {0, 0, 0, 1};
        };
        class KH_IsVictoryTitle: RscText
        {
			text = "Is Victory";
            tooltip = "<true> considers the end a victory.";
            x = QUOTE(RSC_POS_X(6.5));
            y = QUOTE(RSC_POS_Y(7));
            w = QUOTE(RSC_POS_W(6));
            h = QUOTE(RSC_POS_W(1.25));
        };
        class KH_IsVictory: ctrlCheckbox
        {
			idc = 101;
            x = QUOTE(RSC_POS_X(11.5));
            y = QUOTE(RSC_POS_Y(7));
            w = QUOTE(RSC_POS_W(1));
            h = QUOTE(RSC_POS_W(1.25));
            colorBackground[] = {0, 0, 0, 1};
        };
        class KH_FadeTypeTitle: RscText
        {
			text = "Fade Type";
            tooltip = "Type of fade to play after the specified <Delay> time. <-2> plays the signature closing shot and music, <-1> instantly ends the mission without the signature closing shot and music, and any value of <0> and above is a fade to black for that amount of seconds.";
            x = QUOTE(RSC_POS_X(6.5));
            y = QUOTE(RSC_POS_Y(8));
            w = QUOTE(RSC_POS_W(6));
            h = QUOTE(RSC_POS_W(1.25));
        };
        class KH_FadeType: ctrlEdit
        {
			idc = 102;
            x = QUOTE(RSC_POS_X(11.5));
            y = QUOTE(RSC_POS_Y(8));
            w = QUOTE(RSC_POS_W(21));
            h = QUOTE(RSC_POS_W(1.25));
            colorBackground[] = {0, 0, 0, 1};
        };
        class KH_DelayTitle: RscText
        {
			text = "Delay";
            tooltip = "Time to wait before initiating the end of the mission, in seconds. Will clamp to a minimum of 1 second if below <1>.";
            x = QUOTE(RSC_POS_X(6.5));
            y = QUOTE(RSC_POS_Y(9));
            w = QUOTE(RSC_POS_W(6));
            h = QUOTE(RSC_POS_W(1.25));
        };
        class KH_Delay: ctrlEdit
        {
			idc = 103;
            x = QUOTE(RSC_POS_X(11.5));
            y = QUOTE(RSC_POS_Y(9));
            w = QUOTE(RSC_POS_W(21));
            h = QUOTE(RSC_POS_W(1.25));
            colorBackground[] = {0, 0, 0, 1};
        };
        class KH_NextMissionTitle: RscText
        {
			text = "Next Mission";
            tooltip = "An array containing the string of the server password, if any, and the string of the name of the next mission, in format <[_password (STRING), _nextMission (STRING)]>. Can be an empty array in case of no subsequent mission. If this value is empty, or an empty array, it will be ignored. If this value is used, all other values are ignored, and the next mission will start immediately after <Delay> seconds.";
            x = QUOTE(RSC_POS_X(6.5));
            y = QUOTE(RSC_POS_Y(10));
            w = QUOTE(RSC_POS_W(6));
            h = QUOTE(RSC_POS_W(1.25));
        };
        class KH_NextMission: ctrlEdit
        {
			idc = 104;
            x = QUOTE(RSC_POS_X(11.5));
            y = QUOTE(RSC_POS_Y(10));
            w = QUOTE(RSC_POS_W(21));
            h = QUOTE(RSC_POS_W(1.25));
            colorBackground[] = {0, 0, 0, 1};
        };
        class KH_ButtonOK: RscButtonMenuOK 
        {
            x = QUOTE(RSC_POS_X(28.5));
            y = QUOTE(RSC_POS_Y(11.25));
            w = QUOTE(RSC_POS_W(5));
            h = QUOTE(RSC_POS_H(1));
        };
        class KH_ButtonCancel: RscButtonMenuCancel 
        {
            x = QUOTE(RSC_POS_X(6.5));
            y = QUOTE(RSC_POS_Y(11.25));
            w = QUOTE(RSC_POS_W(5));
            h = QUOTE(RSC_POS_H(1));
        };
    };
};