class KH_ConditionalPresence: Title
{
	attributeLoad = "[_this, _value] call KH_fnc_loadControlAttributes;";
	attributeSave = "[_this] call KH_fnc_saveControlAttributes;";
	h = QUOTE(7 * CTRL_DEFAULT_H + 70 * pixelH);
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
		class KH_EventNameTitle: Title
		{
			text = "Event Name";
			tooltip = "Event that dictates the presence of this entity.";
			y = QUOTE(2 * CTRL_DEFAULT_H + 5 * CTRL_DEFAULT_Y);
		};
		class KH_EventName: ctrlEdit
		{
			idc = 101;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(2 * CTRL_DEFAULT_H + 5 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_InitTitle: Title
		{
			text = "Init";
			tooltip = "Unscheduled code to execute locally to this entity when the variable state changes. Useful for adding velocity to vehicles in the air. Passed arguments available through _this are: [_entity (OBJECT), _variableState (BOOLEAN)].";
			y = QUOTE(3 * CTRL_DEFAULT_H + 10 * CTRL_DEFAULT_Y);
		};
		class KH_Init: ctrlEditMulti
		{
			idc = 102;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(3 * CTRL_DEFAULT_H + 10 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H * 3);
		};
		class KH_InvertTitle: Title
		{
			text = "Invert";
			tooltip = "True makes it so that the logic is reversed; the designated variable must be false in order for the entity to become visible and simulated, and true in order for it to become invisible and unsimulated.";
			y = QUOTE(6 * CTRL_DEFAULT_H + 15 * CTRL_DEFAULT_Y);
		};
		class KH_Invert: ctrlCheckbox
		{
			idc = 103;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(6 * CTRL_DEFAULT_H + 15 * CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
	};
};