class KH_PersistentInit: Title
{
	attributeLoad = "[_this, _value] call KH_fnc_loadControlAttributes;";
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
		class KH_LocalFunctionTitle: Title
		{
			text = "Local Function";
			tooltip = "Unscheduled code executed locally to the new owner of the entity. Passed arguments available through <_this> are: [_entity].";
			y = QUOTE(2 * CTRL_DEFAULT_H + 5 * CTRL_DEFAULT_Y);
		};
		class KH_LocalFunction: ctrlEditMulti
		{
			idc = 104;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(2 * CTRL_DEFAULT_H + 5 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H * 3);
		};
		class KH_RemoteFunctionTitle: Title
		{
			text = "Remote Function";
			tooltip = "Unscheduled code executed locally to the previous owner of the entity. Passed arguments available through <_this> are: [_entity].";
			y = QUOTE(5 * CTRL_DEFAULT_H + 10 * CTRL_DEFAULT_Y);
		};
		class KH_RemoteFunction: ctrlEditMulti
		{
			idc = 105;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(5 * CTRL_DEFAULT_H + 10 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H * 3);
		};
	};
};