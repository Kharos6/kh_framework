class KH_ResourceStageExecution
{
    idd = -1;
    function = "KH_fnc_zeusModuleStageExecution";
    movingEnable = 1;
    onLoad = "[_this select 0, 'KH_ResourceStageExecution'] call KH_fnc_loadDisplay";
    class Controls 
    {
        class KH_Title: RscText 
        {
            text = "STAGE EXECUTION";
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
            h = QUOTE(RSC_POS_W(27));
            colorBackground[] = {0, 0, 0, 0.9};
        };
        class KH_Expression: RscEditMulti
        {
			idc = 100;
			font = "EtelkaMonospacePro";
			tooltip = "Unscheduled code to execute on the server when the <INSERT> key is pressed while in the desired display type.\nPassed arguments available through <_this> are: <[_moduleAttachedObject (OBJECT)]>.\n<'CURATOR'> executes the code while in Curator view. <'UNIT'> executes the code while controlling a unit.";
            x = QUOTE(RSC_POS_X(6.5));
            y = QUOTE(RSC_POS_Y(6));
            w = QUOTE(RSC_POS_W(27));
            h = QUOTE(RSC_POS_W(27));
            colorBackground[] = {0, 0, 0, 0};
        };
        class KH_Display: ctrlToolbox
        {
			idc = 101;
            x = QUOTE(RSC_POS_X(6.5));
            y = QUOTE(RSC_POS_Y(26.5));
            w = QUOTE(RSC_POS_W(27));
            h = QUOTE(RSC_POS_W(1.5));
            colorBackground[] = {0, 0, 0, 1};
            rows = 1;
            columns = 2;
            strings[] = 
            {
                "CURATOR",
                "UNIT"
            };
        };
        class KH_ButtonOK: RscButtonMenuOK 
        {
            x = QUOTE(RSC_POS_X(28.5));
            y = QUOTE(RSC_POS_Y(27.5));
            w = QUOTE(RSC_POS_W(5));
            h = QUOTE(RSC_POS_H(1));
        };
        class KH_ButtonCancel: RscButtonMenuCancel 
        {
            x = QUOTE(RSC_POS_X(6.5));
            y = QUOTE(RSC_POS_Y(27.5));
            w = QUOTE(RSC_POS_W(5));
            h = QUOTE(RSC_POS_H(1));
        };
    };
};