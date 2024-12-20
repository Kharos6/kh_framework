class KH_CameraSequence: Title
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
		class KH_PositionsTitle: Title
		{
			text = "Positions";
			tooltip = "Array of arrays that dictate the position of the camera, in format: <[_position (STRING or ARRAY), _attach (BOOLEAN)]>. <_position> is either a string of the variable name of the entity which the position of will be used for the camera, or an <AGL> position array in format: <[x, y, z]>. <_attach> is a boolean where <true> instantly attaches the camera to <_position>, and <false> transitions the camera to <_position>, only effective and necessary if <_position> is an entity.";
			y = QUOTE(2 * CTRL_DEFAULT_H + 5 * CTRL_DEFAULT_Y);
		};
		class KH_Positions: ctrlEdit
		{
			idc = 101;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(2 * CTRL_DEFAULT_H + 5 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_TargetsTitle: Title
		{
			text = "Targets";
			tooltip = "Array of either strings of variable names of entities that the camera will track, or <AGL> position arrays in format <[x, y, z]>.";
			y = QUOTE(3 * CTRL_DEFAULT_H + 10 * CTRL_DEFAULT_Y);
		};
		class KH_Targets: ctrlEdit
		{
			idc = 102;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(3 * CTRL_DEFAULT_H + 10 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_FOVsTitle: Title
		{
			text = "FOVs";
			tooltip = "Array of scalars of FOV values of the camera, from <0.01> to <8.5>. Default FOV is <0.75>.";
			y = QUOTE(4 * CTRL_DEFAULT_H + 15 * CTRL_DEFAULT_Y);
		};
		class KH_FOVs: ctrlEdit
		{
			idc = 103;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(4 * CTRL_DEFAULT_H + 15 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_CommitTimesTitle: Title
		{
			text = "Commit Times";
			tooltip = "Array of scalars dictating the transition duration from one shot to the next, in seconds.";
			y = QUOTE(5 * CTRL_DEFAULT_H + 20 * CTRL_DEFAULT_Y);
		};
		class KH_CommitTimes: ctrlEdit
		{
			idc = 104;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(5 * CTRL_DEFAULT_H + 20 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_DurationsTitle: Title
		{
			text = "Durations";
			tooltip = "Array of scalars dictating the duration of each shot, in seconds.";
			y = QUOTE(6 * CTRL_DEFAULT_H + 25 * CTRL_DEFAULT_Y);
		};
		class KH_Durations: ctrlEdit
		{
			idc = 105;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(6 * CTRL_DEFAULT_H + 25 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_VisionTypesTitle: Title
		{
			text = "Vision Types";
			tooltip = "Array of Scalars that dictate the imaging type of the camera, can be one of the following: <-2> = Night vision. <-1> = Normal vision. <0> = White hot. <1> = Black hot. <2> = Light green hot, darker green cold. <3> = Black hot, darker green cold. <4> = Light red hot, darker red cold. <5> = Black hot, darker red cold. <6> = White Hot, darker red cold. <7> = Thermal shades of red and green, bodies are white.";
			y = QUOTE(7 * CTRL_DEFAULT_H + 30 * CTRL_DEFAULT_Y);
		};
		class KH_VisionTypes: ctrlEdit
		{
			idc = 106;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(7 * CTRL_DEFAULT_H + 30 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_CinematicBordersTitle: Title
		{
			text = "Cinematic Borders";
			tooltip = "<true> adds black bars on the top and bottom of the screen for the duration of the sequence.";
			y = QUOTE(8 * CTRL_DEFAULT_H + 35 * CTRL_DEFAULT_Y);
		};
		class KH_CinematicBorders: ctrlCheckbox
		{
			idc = 107;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(8 * CTRL_DEFAULT_H + 35 * CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_DisableUserInputTitle: Title
		{
			text = "Disable User Input";
			tooltip = "<true> disables user input until the camera sequence is concluded.";
			y = QUOTE(9 * CTRL_DEFAULT_H + 40 * CTRL_DEFAULT_Y);
		};
		class KH_DisableUserInput: ctrlCheckbox
		{
			idc = 108;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(9 * CTRL_DEFAULT_H + 40 * CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_JIPTitle: Title
		{
			text = "JIP";
			tooltip = "<true> plays the camera sequence for players that join while the mission is in progress.";
			y = QUOTE(10 * CTRL_DEFAULT_H + 45 * CTRL_DEFAULT_Y);
		};
		class KH_JIP: ctrlCheckbox
		{
			idc = 109;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(10 * CTRL_DEFAULT_H + 45 * CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
	};
};