class KH_AssignTerminal: Title
{
	attributeLoad = "[_this, _value] call KH_fnc_loadControlAttributes;";
	attributeSave = "[_this] call KH_fnc_saveControlAttributes;";
	h = QUOTE(13 * CTRL_DEFAULT_H + 70 * pixelH);
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
		class KH_NameTitle: Title
		{
			text = "Name";
			tooltip = "Name of the terminal.";
			y = QUOTE(2 * CTRL_DEFAULT_H + 5 * CTRL_DEFAULT_Y);
		};
		class KH_Name: ctrlEdit
		{
			idc = 101;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(2 * CTRL_DEFAULT_H + 5 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_IdentifierTitle: Title
		{
			text = "Identifier";
			tooltip = "Identifier of the terminal. Terminals with the same identifier will share information, but cannot be accessed at the same time.";
			y = QUOTE(3 * CTRL_DEFAULT_H + 10 * CTRL_DEFAULT_Y);
		};
		class KH_Identifier: ctrlEdit
		{
			idc = 102;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(3 * CTRL_DEFAULT_H + 10 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_DescriptionTitle: Title
		{
			text = "Description";
			tooltip = "Description of the terminal, should contain instructions and possible arguments and commands, unless otherwise provided.";
			y = QUOTE(4 * CTRL_DEFAULT_H + 15 * CTRL_DEFAULT_Y);
		};
		class KH_Description: ctrlEditMulti
		{
			idc = 103;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(4 * CTRL_DEFAULT_H + 15 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H * 3);
		};
		class KH_ConditionTitle: Title
		{
			text = "Condition";
			tooltip = "Unscheduled code, executed locally to the player interacting with the terminal, which must return <true> in order for the terminal to be accessible.";
			y = QUOTE(7 * CTRL_DEFAULT_H + 20 * CTRL_DEFAULT_Y);
		};
		class KH_Condition: ctrlEditMulti
		{
			idc = 104;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(7 * CTRL_DEFAULT_H + 20 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H * 3);
		};
		class KH_FunctionTitle: Title
		{
			text = "Function";
			tooltip = "Unscheduled code, executed on the server, which must return a string that will be added on the terminal's console. If no string is returned, nothing is provided to the console output. Passed arguments available through <_this> are: <[_command (STRING), _argument (STRING), _identifierOutput (STRING)]>. <_command> is the first text that a player can insert into the input field of the console, followed by a space and a dash <->, and then the <_argument>. The _command parameter will not include the space, and the <_argument> parameter will not include the dash <->. <_identifierOutput> dictates the text displayed on all terminals sharing the same Identifier, and can be used for extra control through the <KH_fnc_setPersistentDisplayText> function, which accepts the following arguments: <[_identifierOutput (STRING), _text (STRING), _overwrite (BOOLEAN)]>.";
			y = QUOTE(10 * CTRL_DEFAULT_H + 25 * CTRL_DEFAULT_Y);
		};
		class KH_Function: ctrlEditMulti
		{
			idc = 105;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(10 * CTRL_DEFAULT_H + 25 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H * 3);
		};
	};
};