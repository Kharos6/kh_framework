class KH_SetVideoTexture: Title
{
	attributeLoad = "[_this, _value] call KH_fnc_loadControlAttributes;";
	attributeSave = "[_this] call KH_fnc_saveControlAttributes;";
	h = QUOTE(6 * CTRL_DEFAULT_H + 70 * pixelH);
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
		class KH_VideoTitle: Title
		{
			text = "Video";
			tooltip = "Path to the video file.";
			y = QUOTE(2 * CTRL_DEFAULT_H + 5 * CTRL_DEFAULT_Y);
		};
		class KH_Video: ctrlEdit
		{
			idc = 101;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(2 * CTRL_DEFAULT_H + 5 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_TextureTitle: Title
		{
			text = "Texture";
			tooltip = "Texture index to which the video will be applied.";
			y = QUOTE(3 * CTRL_DEFAULT_H + 10 * CTRL_DEFAULT_Y);
		};
		class KH_Texture: ctrlEdit
		{
			idc = 102;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(3 * CTRL_DEFAULT_H + 10 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_AudioTitle: Title
		{
			text = "Audio";
			tooltip = "Class name of audio to play alongside the video. Leave empty for no audio.";
			y = QUOTE(4 * CTRL_DEFAULT_H + 15 * CTRL_DEFAULT_Y);
		};
		class KH_Audio: ctrlEdit
		{
			idc = 103;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(4 * CTRL_DEFAULT_H + 15 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_IntervalTitle: Title
		{
			text = "Interval";
			tooltip = "Interval at which the video and audio will restart.";
			y = QUOTE(5 * CTRL_DEFAULT_H + 20 * CTRL_DEFAULT_Y);
		};
		class KH_Interval: ctrlEdit
		{
			idc = 104;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(5 * CTRL_DEFAULT_H + 20 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
	};
};