class KH_PersistentInit: Title
{
	attributeLoad = "[_this, _value] call KH_fnc_loadControlAttributes;";
	attributeSave = "[_this] call KH_fnc_saveControlAttributes;";
	h = QUOTE(7 * CTRL_DEFAULT_H + 75 * pixelH);
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
		class LocalInitTitle: Title
		{
			text = "Local Init";
			tooltip = "Unscheduled code executed locally to the new owner of the entity. This code is also executed upon loading the mission, and upon load for the player assigned to this entity. Passed arguments available through _this are: [_entity (OBJECT)].";
			y = QUOTE(2 * CTRL_DEFAULT_H + 5 * CTRL_DEFAULT_Y);
		};
		class LocalInit: ctrlEditMulti
		{
			idc = 101;
			font = "EtelkaMonospacePro";
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(2 * CTRL_DEFAULT_H + 5 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H * 3);
		};
		class RemoteInitTitle: Title
		{
			text = "Remote Init";
			tooltip = "Unscheduled code executed locally to the previous owner of the entity. Passed arguments available through _this are: [_entity (OBJECT)].";
			y = QUOTE(5 * CTRL_DEFAULT_H + 10 * CTRL_DEFAULT_Y);
		};
		class RemoteInit: ctrlEditMulti
		{
			idc = 102;
			font = "EtelkaMonospacePro";
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(5 * CTRL_DEFAULT_H + 10 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H * 3);
		};
	};
};