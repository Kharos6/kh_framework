class KH_Persistency: Title
{
	attributeLoad = "(_this controlsGroupCtrl 103) lbAdd 'NONE'; (_this controlsGroupCtrl 103) lbAdd 'SAVED'; (_this controlsGroupCtrl 103) lbAdd 'INITIAL'; (_this controlsGroupCtrl 103) lbAdd 'DEATH'; [_this, _value] call KH_fnc_loadControlAttributes;";
	attributeSave = "[_this] call KH_fnc_saveControlAttributes;";
	h = QUOTE(9 * CTRL_DEFAULT_H + 70 * pixelH);
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
		class IdentifierTitle: Title
		{
			text = "Identifier";
			tooltip = "Identifier from which to derive persistent states, and to which the persistent states will be saved when the KH_fnc_endMission function or the KH End Mission module is executed.";
			y = QUOTE(2 * CTRL_DEFAULT_H + 5 * CTRL_DEFAULT_Y);
		};
		class Identifier: ctrlEdit
		{
			idc = 101;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(2 * CTRL_DEFAULT_H + 5 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class PlayersTitle: Title
		{
			text = "Players";
			tooltip = "True preserves player attributes. Attributes are assigned based on either the Steam ID or variable name, depending on the choice in the unit attributes, and loaded if a player has valid saved attributes. Steam ID is default.";
			y = QUOTE(3 * CTRL_DEFAULT_H + 10 * CTRL_DEFAULT_Y);
		};
		class Players: ctrlCheckbox
		{
			idc = 102;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(3 * CTRL_DEFAULT_H + 10 * CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class PlayerRespawnTypeTitle: Title
		{
			text = "Player Respawn Type";
			tooltip = "Loadout restoration type on player respawn. 'NONE' results in the players getting the default loadout of their unit. 'SAVED' results in the players getting the persistently saved player loadout. If one does not exist, they get the loadout that they started with. 'INITIAL' results in the players getting the loadout that they started with. 'DEATH' results in the players getting the loadout they had when they died. Does not require the player attribute to be true, and thus can be used without the persistency system.";
			y = QUOTE(4 * CTRL_DEFAULT_H + 15 * CTRL_DEFAULT_Y);
		};
		class PlayerRespawnType: ctrlCombo
		{
			idc = 103;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(4 * CTRL_DEFAULT_H + 15 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class UnitsTitle: Title
		{
			text = "Units";
			tooltip = "True preserves unit attributes. Attributes are assigned based on variable names, saved and loaded only if the unit has a variable name assigned.";
			y = QUOTE(5 * CTRL_DEFAULT_H + 20 * CTRL_DEFAULT_Y);
		};
		class Units: ctrlCheckbox
		{
			idc = 104;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(5 * CTRL_DEFAULT_H + 20 * CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class GroupsTitle: Title
		{
			text = "Groups";
			tooltip = "True preserves group attributes. Attributes are assigned based on the group side and ID.";
			y = QUOTE(6 * CTRL_DEFAULT_H + 25 * CTRL_DEFAULT_Y);
		};
		class Groups: ctrlCheckbox
		{
			idc = 105;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(6 * CTRL_DEFAULT_H + 25 * CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class ObjectsTitle: Title
		{
			text = "Objects";
			tooltip = "True preserves object attributes. Attributes are assigned based on variable names, saved and loaded only if the object has a variable name assigned.";
			y = QUOTE(7 * CTRL_DEFAULT_H + 30 * CTRL_DEFAULT_Y);
		};
		class Objects: ctrlCheckbox
		{
			idc = 106;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(7 * CTRL_DEFAULT_H + 30 * CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class MissionTitle: Title
		{
			text = "Mission";
			tooltip = "True preserves the mission state, such as the environmental conditions and date.";
			y = QUOTE(8 * CTRL_DEFAULT_H + 35 * CTRL_DEFAULT_Y);
		};
		class Mission: ctrlCheckbox
		{
			idc = 107;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(8 * CTRL_DEFAULT_H + 35 * CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
	};
};