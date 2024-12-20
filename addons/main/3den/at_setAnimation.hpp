class KH_SetAnimation: Title
{
	attributeLoad = "(_this controlsGroupCtrl 103) lbAdd 'NONE'; (_this controlsGroupCtrl 103) lbAdd 'UNIT'; (_this controlsGroupCtrl 103) lbAdd 'GROUP'; [_this, _value] call KH_fnc_loadControlAttributes;";
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
		class KH_AnimationTitle: Title
		{
			text = "Animation";
			tooltip = "Either the name of the animation to play on the unit, or an array in format of the <switchMove> command syntax.";
			y = QUOTE(2 * CTRL_DEFAULT_H + 5 * CTRL_DEFAULT_Y);
		};
		class KH_Animation: ctrlEdit
		{
			idc = 101;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(2 * CTRL_DEFAULT_H + 5 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_DurationTitle: Title
		{
			text = "Duration";
			tooltip = "Length of the animation, in seconds. <0> is an infinite loop. <-1> freezes the first frame of the chosen animation, then unsimulates and kills the unit. Anything greater than <0> is how long the animation will last before exiting.";
			y = QUOTE(3 * CTRL_DEFAULT_H + 10 * CTRL_DEFAULT_Y);
		};
		class KH_Duration: ctrlEdit
		{
			idc = 102;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(3 * CTRL_DEFAULT_H + 10 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_InterruptTypeTitle: Title
		{
			text = "Interrupt Type";
			tooltip = "<'NONE'> prevents the animation from being interrupted until it is finished. <'UNIT'> interrupts the animation only if the unit is hit. <'GROUP'> interrupts the animation when the unit feels threatened, and the unit's group must be in Safe mode in order to maintain the animation.";
			y = QUOTE(4 * CTRL_DEFAULT_H + 15 * CTRL_DEFAULT_Y);
		};
		class KH_InterruptType: ctrlCombo
		{
			idc = 103;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(4 * CTRL_DEFAULT_H + 15 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
	};
};