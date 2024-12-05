class KH_Teleporter: Title
{
	attributeLoad = "[_this, _value] call KH_fnc_loadControlAttributes;";
	attributeSave = "[_this] call KH_fnc_saveControlAttributes;";
	h = QUOTE(11 * CTRL_DEFAULT_H + 70 * pixelH);
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
		class KH_PositionTitle: Title
		{
			text = "Position";
			tooltip = "Variable name of the entity that will be used as the teleport position.";
			y = QUOTE(2 * CTRL_DEFAULT_H + 5 * CTRL_DEFAULT_Y);
		};
		class KH_Position: ctrlEdit
		{
			idc = 101;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(2 * CTRL_DEFAULT_H + 5 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_RotationTitle: Title
		{
			text = "Rotation";
			tooltip = "Variable name of the entity that will be used as the teleport rotation.";
			y = QUOTE(3 * CTRL_DEFAULT_H + 10 * CTRL_DEFAULT_Y);
		};
		class KH_Rotation: ctrlEdit
		{
			idc = 102;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(3 * CTRL_DEFAULT_H + 10 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_TransitionTitle: Title
		{
			text = "Transition";
			tooltip = "Duration of a teleport transition, where the screen fades out and fades back in to hide the teleport, in seconds.";
			y = QUOTE(4 * CTRL_DEFAULT_H + 15 * CTRL_DEFAULT_Y);
		};
		class KH_Transition: ctrlEdit
		{
			idc = 103;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(4 * CTRL_DEFAULT_H + 15 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_HealTitle: Title
		{
			text = "Heal";
			tooltip = "<true> heals the teleporting players after the teleport.";
			y = QUOTE(5 * CTRL_DEFAULT_H + 20 * CTRL_DEFAULT_Y);
		};
		class KH_Heal: ctrlCheckbox
		{
			idc = 104;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(5 * CTRL_DEFAULT_H + 20 * CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_FreefallHeightTitle: Title
		{
			text = "Freefall Height";
			tooltip = "Freefall height assigned to a teleporting player, in meters. useful for cases where the destination is higher than the default freefall height of 100 meters. Set to <-1> to reset, or set to <0> to avoid any freefall height change.";
			y = QUOTE(6 * CTRL_DEFAULT_H + 25 * CTRL_DEFAULT_Y);
		};
		class KH_FreefallHeight: ctrlEdit
		{
			idc = 105;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(6 * CTRL_DEFAULT_H + 25 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_InitializationTitle: Title
		{
			text = "Initialization";
			tooltip = "Unscheduled code to execute locally to the player after they are teleported. Passed arguments available through <_this> are: [_player].";
			y = QUOTE(7 * CTRL_DEFAULT_H + 30 * CTRL_DEFAULT_Y);
		};
		class KH_Initialization: ctrlEditMulti
		{
			idc = 106;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(7 * CTRL_DEFAULT_H + 30 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H * 3);
		};
		class KH_NameTitle: Title
		{
			text = "Name";
			tooltip = "Name displayed in the teleport action.";
			y = QUOTE(10 * CTRL_DEFAULT_H + 35 * CTRL_DEFAULT_Y);
		};
		class KH_Name: ctrlEdit
		{
			idc = 107;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(10 * CTRL_DEFAULT_H + 35 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
	};
};