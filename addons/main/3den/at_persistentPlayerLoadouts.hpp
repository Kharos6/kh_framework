class KH_PersistentPlayerLoadouts: Title
{
	attributeLoad = "(_this controlsGroupCtrl 102) lbAdd 'NONE'; (_this controlsGroupCtrl 102) lbAdd 'SAVED'; (_this controlsGroupCtrl 102) lbAdd 'INITIAL'; [_this, _value] call KH_fnc_loadControlAttributes;";
	attributeSave = "[_this] call KH_fnc_saveControlAttributes;";
	h = QUOTE(4 * CTRL_DEFAULT_H + 70 * pixelH);
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
		class KH_IdentifierTitle: Title
		{
			text = "Identifier";
			tooltip = "Loadout identifier from which to derive saved player loadouts, and to which player loadouts will be saved when <KH_fnc_endMission> is executed.";
			y = QUOTE(2 * CTRL_DEFAULT_H + 5 * CTRL_DEFAULT_Y);
		};
		class KH_Identifier: ctrlEdit
		{
			idc = 101;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(2 * CTRL_DEFAULT_H + 5 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_RespawnTypeTitle: Title
		{
			text = "Respawn Type";
			tooltip = "Loadout restoration type on player respawn. <NONE> results in the players getting the default loadout of their unit. <SAVED> results in the players getting the saved player loadout. If one does not exist, they get the loadout that they started with. <INITIAL> results in the players getting the loadout that they started with.";
			y = QUOTE(3 * CTRL_DEFAULT_H + 10 * CTRL_DEFAULT_Y);
		};
		class KH_RespawnType: ctrlCombo
		{
			idc = 102;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(3 * CTRL_DEFAULT_H + 10 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
	};
};