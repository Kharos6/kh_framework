class KH_AssignIdentificationCard: Title
{
	attributeLoad = "[_this, _value] call KH_fnc_loadControlAttributes;";
	attributeSave = "[_this] call KH_fnc_saveControlAttributes;";
	h = QUOTE(11 * CTRL_DEFAULT_H + 70 * pixelH);
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
			tooltip = "Name of the person. Can be left empty to derive the name from identity.";
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
		class KH_GenderTitle: Title
		{
			text = "Gender";
			tooltip = "Gender.";
			y = QUOTE(3 * CTRL_DEFAULT_H + 10 * CTRL_DEFAULT_Y);
		};
		class KH_Gender: ctrlEdit
		{
			idc = 102;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(3 * CTRL_DEFAULT_H + 10 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_RaceTitle: Title
		{
			text = "Race";
			tooltip = "Race.";
			y = QUOTE(4 * CTRL_DEFAULT_H + 15 * CTRL_DEFAULT_Y);
		};
		class KH_Race: ctrlEdit
		{
			idc = 103;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(4 * CTRL_DEFAULT_H + 15 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_DateOfBirthTitle: Title
		{
			text = "Date of Birth";
			tooltip = "Date of birth.";
			y = QUOTE(5 * CTRL_DEFAULT_H + 20 * CTRL_DEFAULT_Y);
		};
		class KH_DateOfBirth: ctrlEdit
		{
			idc = 104;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(5 * CTRL_DEFAULT_H + 20 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_ProfessionTitle: Title
		{
			text = "Profession";
			tooltip = "Profession.";
			y = QUOTE(6 * CTRL_DEFAULT_H + 25 * CTRL_DEFAULT_Y);
		};
		class KH_Profession: ctrlEdit
		{
			idc = 105;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(6 * CTRL_DEFAULT_H + 25 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_NationalityTitle: Title
		{
			text = "Nationality";
			tooltip = "Unscheduled code, executed on the server, which must return a string that will be added on the terminal's console. If no string is returned, nothing is provided to the console output. Passed arguments available through <_this> are: <[_command (STRING), _argument (STRING), _identifierOutput (STRING)]>. <_command> is the first text that a player can insert into the input field of the console, followed by a space and a dash <->, and then the <_argument>. The <_command> parameter will not include the space, and the <_argument> parameter will not include the dash <->. <_identifierOutput> dictates the text displayed on all terminals sharing the same Identifier, and can be used for extra control through the <KH_fnc_setPersistentDisplayText> function, which accepts the following arguments: <[_identifierOutput (STRING), _text (STRING), _overwrite (BOOLEAN)]>. Line breaks in text can be added by including a backslash followed by the letter <n>. The terminal output itself can be accessed by external scripts as a <missionNamespace> variable <'KH_var_terminalOutput%1'>, where %1 is the desired identifier.";
			y = QUOTE(7 * CTRL_DEFAULT_H + 30 * CTRL_DEFAULT_Y);
		};
		class KH_Nationality: ctrlEdit
		{
			idc = 106;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(7 * CTRL_DEFAULT_H + 30 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_CardNumberTitle: Title
		{
			text = "Card Number";
			tooltip = "Card number.";
			y = QUOTE(8 * CTRL_DEFAULT_H + 35 * CTRL_DEFAULT_Y);
		};
		class KH_CardNumber: ctrlEdit
		{
			idc = 107;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(8 * CTRL_DEFAULT_H + 35 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_DateOfIssueTitle: Title
		{
			text = "Date of Issue";
			tooltip = "Date of issue.";
			y = QUOTE(9 * CTRL_DEFAULT_H + 40 * CTRL_DEFAULT_Y);
		};
		class KH_DateOfIssue: ctrlEdit
		{
			idc = 108;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(9 * CTRL_DEFAULT_H + 40 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_DateOfExpiryTitle: Title
		{
			text = "Date of Expiry";
			tooltip = "Date of expiry.";
			y = QUOTE(10 * CTRL_DEFAULT_H + 45 * CTRL_DEFAULT_Y);
		};
		class KH_DateOfExpiry: ctrlEdit
		{
			idc = 109;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(10 * CTRL_DEFAULT_H + 45 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
	};
};