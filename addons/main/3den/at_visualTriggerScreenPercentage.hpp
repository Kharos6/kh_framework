class KH_VisualTriggerScreenPercentage: Slider
{
	attributeLoad = "[_this controlsgroupctrl 100, _this controlsgroupctrl 101, '%', _value] call BIS_fnc_initSliderValue;";
	attributeSave = "sliderposition (_this controlsGroupCtrl 100)";
    onLoad = "_ctrlGroup = _this select 0; [_ctrlGroup controlsgroupctrl 100, _ctrlGroup controlsgroupctrl 101, '%'] call BIS_fnc_initSliderValue;";
	h = QUOTE(CTRL_DEFAULT_H + 2 * pixelH);
	class Controls: Controls
	{
		class Title: Title
		{
			text = "Screen Percentage";
			tooltip = "The distance from the edge of the screen to the center of the screen within which the trigger will activate, from 0 to 1, where 0 is pin point center and 1 is the whole screen.";
			y = QUOTE(CTRL_DEFAULT_H * CTRL_DEFAULT_Y);
		};
		class Value: ctrlXSliderH
		{
			idc = 100;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(CTRL_DEFAULT_H * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W * 0.85);
			h = QUOTE(CTRL_DEFAULT_H);
			lineSize = 1;
			sliderRange[] = {0, 1};
			sliderPosition = 0.5;
			sliderStep = 0.01;
		};
		class Edit: ctrlEdit
		{
			idc = 101;
			x = QUOTE(CTRL_DEFAULT_X * 2.455);
			y = QUOTE(CTRL_DEFAULT_H * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W * 0.14875);
			h = QUOTE(CTRL_DEFAULT_H);
		};
	};
};