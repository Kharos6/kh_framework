class KH_ModuleAssignTerminal: Module_F
{
	scope = 2;
	scopeCurator = 1;
	canSetArea = 0;
	canSetAreaHeight = 0;
	canSetAreaShape = 0;
	displayName = "Assign Terminal";
	category = "KH_Modules";
	function = "KH_fnc_moduleAssignTerminal";
	functionPriority = 1;
	isGlobal = 0;
	isTriggerActivated = 0;
	isDisposable = 1;
	is3DEN = 0;
	icon = "\a3\modules_f_curator\data\iconendmission_ca.paa";
	class Attributes: AttributesBase
	{
		class Name: Edit
		{
			displayName = "Name";
			tooltip = "Name of the terminal.";
			property = "KH_ModuleAssignTerminalName";
			defaultValue = "'Terminal'";
		};
		class Identifier: Edit
		{
			displayName = "Identifier";
			tooltip = "Identifier of the terminal. Terminals with the same identifier will share information, but cannot be accessed at the same time.";
			property = "KH_ModuleAssignTerminalIdentifier";
			defaultValue = "''";
		};
		class Description: EditMulti3
		{
			displayName = "Description";
			tooltip = "Description of the terminal. Should ideally contain instructions and possible arguments and commands.";
			property = "KH_ModuleAssignTerminalDescription";
			defaultValue = "''";
		};
		class Condition: EditCodeMulti5
		{
			displayName = "Condition";
			tooltip = "Unscheduled code, executed locally to the player interacting with the terminal, which must return true in order for the terminal to be accessible for that player.";
			property = "KH_ModuleAssignTerminalCondition";
			defaultValue = "''";
		};
		class Function: EditCodeMulti5
		{
			displayName = "Function";
			tooltip = "Unscheduled code, executed on the server, which must return a string that will be added on the terminal's console. If no string is returned, nothing is added to the output. Passed arguments available through <_this> are: [_command (STRING), _argument (STRING), _identifierOutput (STRING)]. _command is the first text that a player can insert into the input field of the console, followed by a space and a dash, and then the _argument. In the context of the provided argument strings, the _command parameter will not include the space and the _argument parameter will not include the dash. _identifierOutput dictates the text displayed on all terminals sharing the same identifier, and can be used for extra control through the KH_fnc_setTerminalText function, which accepts the following arguments: [_identifierOutput (STRING), _text (STRING), _overwrite (BOOLEAN)]. Line breaks in text can be added by including a backslash followed by the letter n. The terminal output itself can be accessed by external scripts as a missionNamespace variable 'KH_var_terminalOutput_%1', where %1 is the desired identifier.";
			property = "KH_ModuleAssignTerminalFunction";
			defaultValue = "''";
		};
		class ModuleDescription: ModuleDescription {};
	};
	class ModuleDescription: ModuleDescription
	{
		description[] = {"Adds an action to the synchronized entities that allows players to open a terminal with a basic input and output system. Activates only once, without a trigger, on the server."};
		sync[] = {"Anything"};
		position = 0;
		direction = 0;
		optional = 0;
		duplicate = 0;
	};
};