class KH_Persistency: Title
{
	attributeLoad = "(_this controlsGroupCtrl 103) lbAdd 'NONE'; (_this controlsGroupCtrl 103) lbAdd 'SAVED'; (_this controlsGroupCtrl 103) lbAdd 'INITIAL'; (_this controlsGroupCtrl 103) lbAdd 'DEATH'; [_this, _value] call KH_fnc_loadControlAttributes;";
	attributeSave = "[_this] call KH_fnc_saveControlAttributes;";
	h = QUOTE(10 * CTRL_DEFAULT_H + 70 * pixelH);
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
		class KH_IdentifierTitle: Title
		{
			text = "Identifier";
			tooltip = "Identifier from which to derive persistent states, and to which the persistent states will be saved when the <KH_fnc_endMission> function, or <End Mission> module, is executed.";
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
		class KH_PlayersTitle: Title
		{
			text = "Players";
			tooltip = "True preserves player loadouts. Loadouts are assigned based on player Steam IDs, and loaded if a player has a valid loadout.";
			y = QUOTE(3 * CTRL_DEFAULT_H + 10 *CTRL_DEFAULT_Y);
		};
		class KH_Players: ctrlCheckbox
		{
			idc = 102;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(3 * CTRL_DEFAULT_H + 10 * CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_PlayerRespawnTypeTitle: Title
		{
			text = "Player Respawn Type";
			tooltip = "Loadout restoration type on player respawn. <'NONE'> results in the players getting the default loadout of their unit. <SAVED> results in the players getting the saved player loadout. If one does not exist, they get the loadout that they started with. <'INITIAL'> results in the players getting the loadout that they started with. <'DEATH'> results in the players getting the loadout they had when they died.";
			y = QUOTE(4 * CTRL_DEFAULT_H + 15 * CTRL_DEFAULT_Y);
		};
		class KH_PlayerRespawnType: ctrlCombo
		{
			idc = 103;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(4 * CTRL_DEFAULT_H + 15 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_ObjectsTitle: Title
		{
			text = "Objects";
			tooltip = "True preserves cargo inventories. They are assigned based on object variable names, saved and loaded only if the object has a variable name and a valid inventory.";
			y = QUOTE(5 * CTRL_DEFAULT_H + 20 *CTRL_DEFAULT_Y);
		};
		class KH_Objects: ctrlCheckbox
		{
			idc = 104;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(5 * CTRL_DEFAULT_H + 20 * CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_WorldTitle: Title
		{
			text = "World";
			tooltip = "True preserves the world state, such as the time and date.";
			y = QUOTE(6 * CTRL_DEFAULT_H + 25 *CTRL_DEFAULT_Y);
		};
		class KH_World: ctrlCheckbox
		{
			idc = 105;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(6 * CTRL_DEFAULT_H + 25 * CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_VariablesTitle: Title
		{
			text = "Variables";
			tooltip = "Array of strings of server <missionNamespace> variables to preserve upon saving, and define with the saved values upon load.";
			y = QUOTE(7 * CTRL_DEFAULT_H + 30 * CTRL_DEFAULT_Y);
		};
		class KH_Variables: ctrlEditMulti
		{
			idc = 106;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(7 * CTRL_DEFAULT_H + 30 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H * 3);
		};
	};
};