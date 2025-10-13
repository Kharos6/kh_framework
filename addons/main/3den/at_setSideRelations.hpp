class KH_SetSideRelations: Title
{
	attributeLoad = "[_this, _value] call KH_fnc_loadControlAttributes;";
	attributeSave = "[_this] call KH_fnc_saveControlAttributes;";
	h = QUOTE(19 * CTRL_DEFAULT_H + 70 * pixelH);
	class Controls: Controls
	{
		class KH_ToggleTitle: Title
		{
			text = "Toggle";
			tooltip = "True allows this function to execute.";
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
		class KH_BluforToBluforTitle: Title
		{
			text = "BLUFOR To BLUFOR";
			tooltip = "<true> makes BLUFOR units friendly to BLUFOR units.";
			y = QUOTE(2 * CTRL_DEFAULT_H + 5 * CTRL_DEFAULT_Y);
		};
		class KH_BluforToBlufor: ctrlCheckbox
		{
			idc = 101;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(2 * CTRL_DEFAULT_H + 5 * CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_BluforToOpforTitle: Title
		{
			text = "BLUFOR To OPFOR";
			tooltip = "<true> makes BLUFOR units friendly to OPFOR units.";
			y = QUOTE(3 * CTRL_DEFAULT_H + 10 * CTRL_DEFAULT_Y);
		};
		class KH_BluforToOpfor: ctrlCheckbox
		{
			idc = 102;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(3 * CTRL_DEFAULT_H + 10 * CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_BluforToGreenforTitle: Title
		{
			text = "BLUFOR To GREENFOR";
			tooltip = "<true> makes BLUFOR units friendly to GREENFOR units.";
			y = QUOTE(4 * CTRL_DEFAULT_H + 15 * CTRL_DEFAULT_Y);
		};
		class KH_BluforToGreenfor: ctrlCheckbox
		{
			idc = 103;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(4 * CTRL_DEFAULT_H + 15 * CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_BluforToCivilianTitle: Title
		{
			text = "BLUFOR To Civilian";
			tooltip = "<true> makes BLUFOR units friendly to Civilian units.";
			y = QUOTE(5 * CTRL_DEFAULT_H + 20 * CTRL_DEFAULT_Y);
		};
		class KH_BluforToCivilian: ctrlCheckbox
		{
			idc = 104;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(5 * CTRL_DEFAULT_H + 20 * CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_OpforToBluforTitle: Title
		{
			text = "OPFOR To BLUFOR";
			tooltip = "<true> makes OPFOR units friendly to BLUFOR units.";
			y = QUOTE(6 * CTRL_DEFAULT_H + 25 * CTRL_DEFAULT_Y);
		};
		class KH_OpforToBlufor: ctrlCheckbox
		{
			idc = 105;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(6 * CTRL_DEFAULT_H + 25 * CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_OpforToOpforTitle: Title
		{
			text = "OPFOR To OPFOR";
			tooltip = "<true> makes OPFOR units friendly to OPFOR units.";
			y = QUOTE(7 * CTRL_DEFAULT_H + 30 * CTRL_DEFAULT_Y);
		};
		class KH_OpforToOpfor: ctrlCheckbox
		{
			idc = 106;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(7 * CTRL_DEFAULT_H + 30 * CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_OpforToGreenforTitle: Title
		{
			text = "OPFOR To GREENFFOR";
			tooltip = "<true> makes OPFOR units friendly to GREENFOR units.";
			y = QUOTE(8 * CTRL_DEFAULT_H + 35 * CTRL_DEFAULT_Y);
		};
		class KH_OpforToGreenfor: ctrlCheckbox
		{
			idc = 107;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(8 * CTRL_DEFAULT_H + 35 * CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_OpforToCivilianTitle: Title
		{
			text = "OPFOR To Civilian";
			tooltip = "<true> makes OPFOR units friendly to Civilian units.";
			y = QUOTE(9 * CTRL_DEFAULT_H + 40 * CTRL_DEFAULT_Y);
		};
		class KH_OpforToCivilian: ctrlCheckbox
		{
			idc = 108;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(9 * CTRL_DEFAULT_H + 40 * CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_GreenforToBluforTitle: Title
		{
			text = "GREENFOR To BLUFOR";
			tooltip = "<true> makes GREENFOR units friendly to BLUFOR units.";
			y = QUOTE(10 * CTRL_DEFAULT_H + 45 * CTRL_DEFAULT_Y);
		};
		class KH_GreenforToBlufor: ctrlCheckbox
		{
			idc = 109;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(10 * CTRL_DEFAULT_H + 45 * CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_GreenforToOpforTitle: Title
		{
			text = "GREENFOR To OPFOR";
			tooltip = "<true> makes GREENFOR units friendly to OPFOR units.";
			y = QUOTE(11 * CTRL_DEFAULT_H + 50 * CTRL_DEFAULT_Y);
		};
		class KH_GreenforToOpfor: ctrlCheckbox
		{
			idc = 110;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(11 * CTRL_DEFAULT_H + 50 * CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_GreenforToGreenforTitle: Title
		{
			text = "GREENFOR To GREENFOR";
			tooltip = "<true> makes GREENFOR units friendly to GREENFOR units.";
			y = QUOTE(12 * CTRL_DEFAULT_H + 55 * CTRL_DEFAULT_Y);
		};
		class KH_GreenforToGreenfor: ctrlCheckbox
		{
			idc = 111;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(12 * CTRL_DEFAULT_H + 55 * CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_GreenforToCivilianTitle: Title
		{
			text = "GREENFOR To Civilian";
			tooltip = "<true> makes GREENFOR units friendly to Civilian units.";
			y = QUOTE(13 * CTRL_DEFAULT_H + 60 * CTRL_DEFAULT_Y);
		};
		class KH_GreenforToCivilian: ctrlCheckbox
		{
			idc = 112;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(13 * CTRL_DEFAULT_H + 60 * CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_CivilianToBluforTitle: Title
		{
			text = "Civilian To BLUFOR";
			tooltip = "<true> makes Civilian units friendly to BLUFOR units.";
			y = QUOTE(14 * CTRL_DEFAULT_H + 65 * CTRL_DEFAULT_Y);
		};
		class KH_CivilianToBlufor: ctrlCheckbox
		{
			idc = 113;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(14 * CTRL_DEFAULT_H + 65 * CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_CivilianToOpforTitle: Title
		{
			text = "Civilian To OPFOR";
			tooltip = "<true> makes Civilian units friendly to OPFOR units.";
			y = QUOTE(15 * CTRL_DEFAULT_H + 70 * CTRL_DEFAULT_Y);
		};
		class KH_CivilianToOpfor: ctrlCheckbox
		{
			idc = 114;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(15 * CTRL_DEFAULT_H + 70 * CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_CivilianToGreenforTitle: Title
		{
			text = "Civilian To GREENFOR";
			tooltip = "<true> makes Civilian units friendly to GREENFOR units.";
			y = QUOTE(16 * CTRL_DEFAULT_H + 75 * CTRL_DEFAULT_Y);
		};
		class KH_CivilianToGreenfor: ctrlCheckbox
		{
			idc = 115;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(16 * CTRL_DEFAULT_H + 75 * CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_CivilianToCivilianTitle: Title
		{
			text = "Civilian To Civilian";
			tooltip = "<true> makes Civilian units friendly to Civilian units.";
			y = QUOTE(17 * CTRL_DEFAULT_H + 80 * CTRL_DEFAULT_Y);
		};
		class KH_CivilianToCivilian: ctrlCheckbox
		{
			idc = 116;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(17 * CTRL_DEFAULT_H + 80 * CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
	};
};