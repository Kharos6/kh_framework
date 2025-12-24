class KH_SideRelations: Title
{
	attributeLoad = "[_this, _value] call KH_fnc_loadControlAttributes;";
	attributeSave = "[_this] call KH_fnc_saveControlAttributes;";
	h = QUOTE(18 * CTRL_DEFAULT_H + 80 * pixelH);
	class Controls: Controls
	{
		class ToggleTitle: Title
		{
			text = "Toggle";
			tooltip = "True allows this function to execute.";
			y = QUOTE(0 * CTRL_DEFAULT_H + CTRL_DEFAULT_Y);
		};
		class Toggle: ctrlCheckbox
		{
			idc = 100;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(0 * CTRL_DEFAULT_H + CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class BluforToBluforTitle: Title
		{
			text = "BLUFOR To BLUFOR";
			tooltip = "True makes BLUFOR units friendly to BLUFOR units.";
			y = QUOTE(2 * CTRL_DEFAULT_H + 5 * CTRL_DEFAULT_Y);
		};
		class BluforToBlufor: ctrlCheckbox
		{
			idc = 101;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(2 * CTRL_DEFAULT_H + 5 * CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class BluforToOpforTitle: Title
		{
			text = "BLUFOR To OPFOR";
			tooltip = "True makes BLUFOR units friendly to OPFOR units.";
			y = QUOTE(3 * CTRL_DEFAULT_H + 10 * CTRL_DEFAULT_Y);
		};
		class BluforToOpfor: ctrlCheckbox
		{
			idc = 102;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(3 * CTRL_DEFAULT_H + 10 * CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class BluforToGreenforTitle: Title
		{
			text = "BLUFOR To GREENFOR";
			tooltip = "True makes BLUFOR units friendly to GREENFOR units.";
			y = QUOTE(4 * CTRL_DEFAULT_H + 15 * CTRL_DEFAULT_Y);
		};
		class BluforToGreenfor: ctrlCheckbox
		{
			idc = 103;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(4 * CTRL_DEFAULT_H + 15 * CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class BluforToCivilianTitle: Title
		{
			text = "BLUFOR To Civilian";
			tooltip = "True makes BLUFOR units friendly to Civilian units.";
			y = QUOTE(5 * CTRL_DEFAULT_H + 20 * CTRL_DEFAULT_Y);
		};
		class BluforToCivilian: ctrlCheckbox
		{
			idc = 104;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(5 * CTRL_DEFAULT_H + 20 * CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class OpforToBluforTitle: Title
		{
			text = "OPFOR To BLUFOR";
			tooltip = "True makes OPFOR units friendly to BLUFOR units.";
			y = QUOTE(6 * CTRL_DEFAULT_H + 25 * CTRL_DEFAULT_Y);
		};
		class OpforToBlufor: ctrlCheckbox
		{
			idc = 105;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(6 * CTRL_DEFAULT_H + 25 * CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class OpforToOpforTitle: Title
		{
			text = "OPFOR To OPFOR";
			tooltip = "True makes OPFOR units friendly to OPFOR units.";
			y = QUOTE(7 * CTRL_DEFAULT_H + 30 * CTRL_DEFAULT_Y);
		};
		class OpforToOpfor: ctrlCheckbox
		{
			idc = 106;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(7 * CTRL_DEFAULT_H + 30 * CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class OpforToGreenforTitle: Title
		{
			text = "OPFOR To GREENFFOR";
			tooltip = "True makes OPFOR units friendly to GREENFOR units.";
			y = QUOTE(8 * CTRL_DEFAULT_H + 35 * CTRL_DEFAULT_Y);
		};
		class OpforToGreenfor: ctrlCheckbox
		{
			idc = 107;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(8 * CTRL_DEFAULT_H + 35 * CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class OpforToCivilianTitle: Title
		{
			text = "OPFOR To Civilian";
			tooltip = "True makes OPFOR units friendly to Civilian units.";
			y = QUOTE(9 * CTRL_DEFAULT_H + 40 * CTRL_DEFAULT_Y);
		};
		class OpforToCivilian: ctrlCheckbox
		{
			idc = 108;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(9 * CTRL_DEFAULT_H + 40 * CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class GreenforToBluforTitle: Title
		{
			text = "GREENFOR To BLUFOR";
			tooltip = "True makes GREENFOR units friendly to BLUFOR units.";
			y = QUOTE(10 * CTRL_DEFAULT_H + 45 * CTRL_DEFAULT_Y);
		};
		class GreenforToBlufor: ctrlCheckbox
		{
			idc = 109;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(10 * CTRL_DEFAULT_H + 45 * CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class GreenforToOpforTitle: Title
		{
			text = "GREENFOR To OPFOR";
			tooltip = "True makes GREENFOR units friendly to OPFOR units.";
			y = QUOTE(11 * CTRL_DEFAULT_H + 50 * CTRL_DEFAULT_Y);
		};
		class GreenforToOpfor: ctrlCheckbox
		{
			idc = 110;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(11 * CTRL_DEFAULT_H + 50 * CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class GreenforToGreenforTitle: Title
		{
			text = "GREENFOR To GREENFOR";
			tooltip = "True makes GREENFOR units friendly to GREENFOR units.";
			y = QUOTE(12 * CTRL_DEFAULT_H + 55 * CTRL_DEFAULT_Y);
		};
		class GreenforToGreenfor: ctrlCheckbox
		{
			idc = 111;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(12 * CTRL_DEFAULT_H + 55 * CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class GreenforToCivilianTitle: Title
		{
			text = "GREENFOR To Civilian";
			tooltip = "True makes GREENFOR units friendly to Civilian units.";
			y = QUOTE(13 * CTRL_DEFAULT_H + 60 * CTRL_DEFAULT_Y);
		};
		class GreenforToCivilian: ctrlCheckbox
		{
			idc = 112;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(13 * CTRL_DEFAULT_H + 60 * CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class CivilianToBluforTitle: Title
		{
			text = "Civilian To BLUFOR";
			tooltip = "True makes Civilian units friendly to BLUFOR units.";
			y = QUOTE(14 * CTRL_DEFAULT_H + 65 * CTRL_DEFAULT_Y);
		};
		class CivilianToBlufor: ctrlCheckbox
		{
			idc = 113;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(14 * CTRL_DEFAULT_H + 65 * CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class CivilianToOpforTitle: Title
		{
			text = "Civilian To OPFOR";
			tooltip = "True makes Civilian units friendly to OPFOR units.";
			y = QUOTE(15 * CTRL_DEFAULT_H + 70 * CTRL_DEFAULT_Y);
		};
		class CivilianToOpfor: ctrlCheckbox
		{
			idc = 114;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(15 * CTRL_DEFAULT_H + 70 * CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class CivilianToGreenforTitle: Title
		{
			text = "Civilian To GREENFOR";
			tooltip = "True makes Civilian units friendly to GREENFOR units.";
			y = QUOTE(16 * CTRL_DEFAULT_H + 75 * CTRL_DEFAULT_Y);
		};
		class CivilianToGreenfor: ctrlCheckbox
		{
			idc = 115;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(16 * CTRL_DEFAULT_H + 75 * CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class CivilianToCivilianTitle: Title
		{
			text = "Civilian To Civilian";
			tooltip = "True makes Civilian units friendly to Civilian units.";
			y = QUOTE(17 * CTRL_DEFAULT_H + 80 * CTRL_DEFAULT_Y);
		};
		class CivilianToCivilian: ctrlCheckbox
		{
			idc = 116;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(17 * CTRL_DEFAULT_H + 80 * CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
	};
};