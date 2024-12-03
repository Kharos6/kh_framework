class KH_EnvironmentAmbiance: Title
{
	attributeLoad = "[_this controlsGroupCtrl 103, _this controlsGroupCtrl 104, '%'] call BIS_fnc_initSliderValue; [_this controlsGroupCtrl 103, _this controlsGroupCtrl 104, '%', _value select 3] call BIS_fnc_initSliderValue; [_this, _value] call KH_fnc_loadControlAttributes;";
	attributeSave = "[_this] call KH_fnc_saveControlAttributes;";
	h = QUOTE(5 * CTRL_DEFAULT_H + 70 * pixelH);
	class Controls: Controls
	{
		class KH_ToggleTitle: Title
		{
			text = "Toggle";
			tooltip = "<true> allows this function to execute.";
			y = QUOTE(0 * CTRL_DEFAULT_H + CTRL_DEFAULT_Y);
		};
		class KH_Toggle: ctrlCheckbox
		{
			idc = 100;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(0 * CTRL_DEFAULT_H + CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_AmbientLifeTitle: Title
		{
			text = "Ambient Life";
			tooltip = "<false> eliminates the presence of ambient life, such as bees, rabbits, snakes, and fish.";
			y = QUOTE(2 * CTRL_DEFAULT_H + 5 * CTRL_DEFAULT_Y);
		};
		class KH_AmbientLife: ctrlCheckbox
		{
			idc = 101;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(2 * CTRL_DEFAULT_H + 5 * CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_AmbientSoundsTitle: Title
		{
			text = "Ambient Sounds";
			tooltip = "<false> eliminates the presence of ambient sounds, such as owls and birds.";
			y = QUOTE(3 * CTRL_DEFAULT_H + 10 * CTRL_DEFAULT_Y);
		};
		class KH_AmbientSounds: ctrlCheckbox
		{
			idc = 102;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(3 * CTRL_DEFAULT_H + 10 * CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_WindCoefficientTitle: Title
		{
			text = "Wind Coefficient";
			tooltip = "Percentage of howling wind presence at heights.";
			y = QUOTE(4 * CTRL_DEFAULT_H + 15 * CTRL_DEFAULT_Y);
		};
		class KH_WindCoefficient: ctrlXSliderH
		{
			idc = 103;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(4 * CTRL_DEFAULT_H + 15 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W * 0.85);
			h = QUOTE(CTRL_DEFAULT_H);
        	sliderRange[] = {0, 1};
       		sliderStep = 0.01;
        	lineSize = 1;
		};
		class KH_WindCoefficientEdit: ctrlEdit
		{
			idc = 104;
			x = QUOTE(CTRL_DEFAULT_X * 2.455);
			y = QUOTE(4 * CTRL_DEFAULT_H + 15 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W * 0.15);
			h = QUOTE(CTRL_DEFAULT_H);
		};
	};
};