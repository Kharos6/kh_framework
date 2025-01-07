class KH_Dynamic3DAudio: Title
{
	attributeLoad = "[_this controlsGroupCtrl 105, _this controlsGroupCtrl 106, '%'] call BIS_fnc_initSliderValue; [_this controlsGroupCtrl 105, _this controlsGroupCtrl 106, '%', _value select 5] call BIS_fnc_initSliderValue; [_this, _value] call KH_fnc_loadControlAttributes;";
	attributeSave = "[_this] call KH_fnc_saveControlAttributes;";
	h = QUOTE(8 * CTRL_DEFAULT_H + 70 * pixelH);
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
		class KH_AudioTitle: Title
		{
			text = "Audio";
			tooltip = "Array of either strings of audio class names, or arrays containing the strings of audio class names, in format of the <say3D> command syntax. These are randomly selected to play.";
			y = QUOTE(2 * CTRL_DEFAULT_H + 5 * CTRL_DEFAULT_Y);
		};
		class KH_Audio: ctrlEdit
		{
			idc = 101;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(2 * CTRL_DEFAULT_H + 5 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_ConditionTitle: Title
		{
			text = "Condition";
			tooltip = "Unscheduled code, executed on the server, which must return <true> in order to play the audio. Checked every time the audio has a chance to play, individually for each player. Passed arguments available through <_this> are: <[_player (OBJECT), _emitter (OBJECT), _selectedAudio (ARRAY or STRING), _handlerId (ARRAY)]>.";
			y = QUOTE(3 * CTRL_DEFAULT_H + 10 * CTRL_DEFAULT_Y);
		};
		class KH_Condition: ctrlEditMulti
		{
			idc = 102;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(3 * CTRL_DEFAULT_H + 10 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H * 3);
		};
		class KH_IntervalTitle: Title
		{
			text = "Interval";
			tooltip = "Interval at which the audio has a chance to play, in seconds.";
			y = QUOTE(6 * CTRL_DEFAULT_H + 15 * CTRL_DEFAULT_Y);
		};
		class KH_Interval: ctrlEdit
		{
			idc = 103;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(6 * CTRL_DEFAULT_H + 15 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_RandomDelayTitle: Title
		{
			text = "Random Delay";
			tooltip = "An amount of seconds, from <0> to desired amount, that will be picked at random, which will determine a delay after which the sound will play on each interval. Set to <0> for no delay.";
			y = QUOTE(7 * CTRL_DEFAULT_H + 20 * CTRL_DEFAULT_Y);
		};
		class KH_RandomDelay: ctrlEdit
		{
			idc = 104;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(7 * CTRL_DEFAULT_H + 20 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_ChanceTitle: Title
		{
			text = "Chance";
			tooltip = "Chance that the audio will play at each interval.";
			y = QUOTE(8 * CTRL_DEFAULT_H + 25 * CTRL_DEFAULT_Y);
		};
		class KH_Chance: ctrlXSliderH
		{
			idc = 105;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(8 * CTRL_DEFAULT_H + 25 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W * 0.85);
			h = QUOTE(CTRL_DEFAULT_H);
        	sliderRange[] = {0, 1};
       		sliderStep = 0.01;
        	lineSize = 1;
		};
		class KH_ChanceEdit: ctrlEdit
		{
			idc = 106;
			x = QUOTE(CTRL_DEFAULT_X * 2.455);
			y = QUOTE(8 * CTRL_DEFAULT_H + 25 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W * 0.15);
			h = QUOTE(CTRL_DEFAULT_H);
		};
	};
};