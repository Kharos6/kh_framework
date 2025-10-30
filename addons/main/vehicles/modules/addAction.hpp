class KH_ModuleAddAction: Module_F
{
	author = "Kharos";
	scope = 2;
	scopeCurator = 1;
	canSetArea = 0;
	canSetAreaHeight = 0;
	canSetAreaShape = 0;
	displayName = "Add Action";
	category = "KH_Modules";
	function = "KH_fnc_moduleAddAction";
	functionPriority = 1;
	isGlobal = 0;
	isTriggerActivated = 0;
	isDisposable = 1;
	is3DEN = 0;
	curatorInfoType = "";
	curatorInfoTypeEmpty = "";
	curatorCanAttach = 0;
	icon = "iconmodule";
	class Attributes: AttributesBase
	{
		class KH_ModuleAddActionHandleObjectActionRecovery: Checkbox
		{
			displayName = "Handle Object Action Recovery";
			tooltip = "True recovers the action on the affected objects if they respawn.";
			property = "KH_ModuleAddActionHandleObjectActionRecovery";
			defaultValue = "true";
		};
		class KH_ModuleAddActionName: Edit
		{
			displayName = "Name";
			tooltip = "Name of the action.";
			property = "KH_ModuleAddActionName";
			defaultValue = "''";
		};
		class KH_ModuleAddActionFunctionStart: EditCodeMulti5
		{
			displayName = "Function Start";
			tooltip = "Unscheduled code executed locally to the caller when the caller initiates the action. Local variables exposed to the function are: _caller (OBJECT), _target (OBJECT), _actionId (SCALAR), _handlerId (ARRAY), _duration (BOOL, SCALAR), _resultPreviousStart, _resultPreviousProgress, _resultPreviousInterrupt, _resultPreviousCancel, and _resultPreviousComplete. The _duration variable may be changed to alter the duration of the action for the current execution.";
			property = "KH_ModuleAddActionFunctionStart";
			defaultValue = "''";
		};
		class KH_ModuleAddActionFunctionProgress: EditCodeMulti5
		{
			displayName = "Function Progress";
			tooltip = "Unscheduled code executed locally to the caller on each frame as the action progresses through its duration. Local variables exposed to the function are: _caller (OBJECT), _target (OBJECT), _actionId (SCALAR), _handlerId (ARRAY), _resultPreviousStart, _resultPreviousProgress, _resultPreviousInterrupt, _resultPreviousCancel, _resultPreviousComplete, _resultStart, and _resultProgress.";
			property = "KH_ModuleAddActionFunctionProgress";
			defaultValue = "''";
		};
		class KH_ModuleAddActionFunctionInterrupt: EditCodeMulti5
		{
			displayName = "Function Interrupt";
			tooltip = "Unscheduled code executed locally to the caller when the action is interrupted in the event of a condition failure, when the caller fails the detection check, or the caller moves beyond the designated distance. Local variables exposed to the function are: _caller (OBJECT), _target (OBJECT), _actionId (SCALAR), _handlerId (ARRAY), _resultPreviousStart, _resultPreviousProgress, _resultPreviousInterrupt, _resultPreviousCancel, _resultPreviousComplete, _resultStart, and _resultProgress.";
			property = "KH_ModuleAddActionFunctionInterrupt";
			defaultValue = "''";
		};
		class KH_ModuleAddActionFunctionCancel: EditCodeMulti5
		{
			displayName = "Function Cancel";
			tooltip = "Unscheduled code executed locally to the caller when the action is cancelled by the caller. Local variables exposed to the function are: _caller (OBJECT), _target (OBJECT), _actionId (SCALAR), _handlerId (ARRAY), _resultPreviousStart, _resultPreviousProgress, _resultPreviousInterrupt, _resultPreviousCancel, _resultPreviousComplete, _resultStart, and _resultProgress.";
			property = "KH_ModuleAddActionFunctionCancel";
			defaultValue = "''";
		};
		class KH_ModuleAddActionFunctionComplete: EditCodeMulti5
		{
			displayName = "Function Complete";
			tooltip = "Unscheduled code executed locally to the caller when the action is completed. Local variables exposed to the function are: _caller (OBJECT), _target (OBJECT), _actionId (SCALAR), _handlerId (ARRAY), _resultStart, _resultPreviousStart, _resultPreviousProgress, _resultPreviousInterrupt, _resultPreviousCancel, _resultPreviousComplete, and _resultProgress.";
			property = "KH_ModuleAddActionFunctionComplete";
			defaultValue = "''";
		};
		class KH_ModuleAddActionConditionExist: EditCodeMulti5
		{
			displayName = "Condition Exist";
			tooltip = "Unscheduled code, executed locally to the creator of the action, in this case the server, that must return true in order for the action to exist. Can also be a string representation of a variable, an array of strings of multiple variables in the affected object's namespace that must be true, or an array of arrays of variables that must return the wanted value in format [variableName (STRING), wantedValue (BOOL, SCALAR, STRING, ARRAY, HASHMAP)], as simply as 'myVariable', or ['myVariable1', 'myVariable2', 'myVariable3', ...] or [['myVariable1', wantedValue1], ['myVariable2', wantedValue2], ['myVariable3', wantedValue3], ...]. If no objects are synchronized, missionNamespace is used to validate the variables instead.";
			property = "KH_ModuleAddActionConditionExist";
			defaultValue = "'true'";
		};
		class KH_ModuleAddActionConditionShow: EditCodeMulti5
		{
			displayName = "Condition Show";
			tooltip = "Unscheduled code, executed locally to the caller, that must return true in order for the action to be visible. Can also be a string representation of a variable, an array of strings of multiple variables in an affected object's namespace that must be true, or an array of arrays of variables that must return the wanted value in format [variableName (STRING), wantedValue (BOOL, SCALAR, STRING, ARRAY, HASHMAP)], as simply as 'myVariable', or ['myVariable1', 'myVariable2', 'myVariable3', ...] or [['myVariable1', wantedValue1], ['myVariable2', wantedValue2], ['myVariable3', wantedValue3], ...]. Local variables exposed to the function are: _caller (OBJECT), _target (OBJECT), _actionId (SCALAR), _handlerId (ARRAY), _resultPreviousStart, _resultPreviousProgress, _resultPreviousInterrupt, _resultPreviousCancel, and _resultPreviousComplete.";
			property = "KH_ModuleAddActionConditionShow";
			defaultValue = "'true'";
		};
		class KH_ModuleAddActionConditionStart: EditCodeMulti5
		{
			displayName = "Condition Start";
			tooltip = "Unscheduled code, executed locally to the caller, that must return true in order for the action to start. Can also be a string representation of a variable, an array of strings of multiple variables in an affected object's namespace that must be true, or an array of arrays of variables that must return the wanted value in format [variableName (STRING), wantedValue (BOOL, SCALAR, STRING, ARRAY, HASHMAP)], as simply as 'myVariable', or ['myVariable1', 'myVariable2', 'myVariable3', ...] or [['myVariable1', wantedValue1], ['myVariable2', wantedValue2], ['myVariable3', wantedValue3], ...]. Local variables exposed to the function are: _caller (OBJECT), _target (OBJECT), _actionId (SCALAR), _handlerId (ARRAY), _duration (BOOL, SCALAR), _resultPreviousStart, _resultPreviousProgress, _resultPreviousInterrupt, _resultPreviousCancel, and _resultPreviousComplete. The _duration variable may be changed to alter the duration of the action for the current execution.";
			property = "KH_ModuleAddActionConditionStart";
			defaultValue = "'true'";
		};
		class KH_ModuleAddActionConditionProgress: EditCodeMulti5
		{
			displayName = "Condition Progress";
			tooltip = "Unscheduled code, executed locally to the caller, that must return true in order for the action to progress through its duration. Can also be a string representation of a variable, an array of strings of multiple variables in an affected object's namespace that must be true, or an array of arrays of variables that must return the wanted value in format [variableName (STRING), wantedValue (BOOL, SCALAR, STRING, ARRAY, HASHMAP)], as simply as 'myVariable', or ['myVariable1', 'myVariable2', 'myVariable3', ...] or [['myVariable1', wantedValue1], ['myVariable2', wantedValue2], ['myVariable3', wantedValue3], ...]. Local variables exposed to the function are: _caller (OBJECT), _target (OBJECT), _actionId (SCALAR), _handlerId (ARRAY), _resultPreviousStart, _resultPreviousProgress, _resultPreviousInterrupt, _resultPreviousCancel, _resultPreviousComplete, _resultStart, and _resultProgress.";
			property = "KH_ModuleAddActionConditionProgress";
			defaultValue = "'true'";
		};
		class KH_ModuleAddActionConditionComplete: EditCodeMulti5
		{
			displayName = "Condition Complete";
			tooltip = "Unscheduled code, executed locally to the caller, that must return true in order for the action to be allowed to complete. Can also be a string representation of a variable, an array of strings of multiple variables in an affected object's namespace that must be true, or an array of arrays of variables that must return the wanted value in format [variableName (STRING), wantedValue (BOOL, SCALAR, STRING, ARRAY, HASHMAP)] of variables that must return the wanted value, as simply as 'myVariable' or ['myVariable1', 'myVariable2', 'myVariable3', ...] or [['myVariable1', wantedValue1], ['myVariable2', wantedValue2], ['myVariable3', wantedValue3], ...]. Local variables exposed to the function are: _caller (OBJECT), _target (OBJECT), _actionId (SCALAR), _handlerId (ARRAY), _resultPreviousStart, _resultPreviousProgress, _resultPreviousInterrupt, _resultPreviousCancel, _resultPreviousComplete, _resultStart, and _resultProgress.";
			property = "KH_ModuleAddActionConditionComplete";
			defaultValue = "'true'";
		};
		class KH_ModuleAddActionRepeatable: Checkbox
		{
			displayName = "Repeatable";
			tooltip = "True allows the action to be called recursively after completion.";
			property = "KH_ModuleAddActionRepeatable";
			defaultValue = "false";
		};
		class KH_ModuleAddActionExclusive: Checkbox
		{
			displayName = "Exclusive";
			tooltip = "True prevents multiple players from being able to call the action at the same time.";
			property = "KH_ModuleAddActionExclusive";
			defaultValue = "true";
		};
		class KH_ModuleAddActionJIP: Checkbox
		{
			displayName = "JIP";
			tooltip = "True makes the action available for players who join while the mission is in progress.";
			property = "KH_ModuleAddActionJIP";
			defaultValue = "true";
		};
		class KH_ModuleAddActionDuration: Edit
		{
			displayName = "Duration";
			tooltip = "Time it takes for the action to complete after being called. Set to 0 for instant completion, -1 for infinite progression that is useful for operation length defined by the caller, or any positive number for duration in seconds. The action can be cancelled by selecting the action again while it is progressing in case of positive duration.";
			property = "KH_ModuleAddActionDuration";
			defaultValue = "'0'";
		};
		class KH_ModuleAddActionProgressDisplay: Combo
		{
			displayName = "Progress Display";
			tooltip = "The type of visual progress indicator to display to the caller through the duration of the action.";
			property = "KH_ModuleAddActionProgressDisplay";
			typeName = "STRING";
			defaultValue = "'NONE'";
			class Values
			{
				class KH_None {name = "NONE"; value = "NONE";};
				class KH_Bar {name = "BAR"; value = "BAR";};
				class KH_Percentage {name = "PERCENTAGE"; value = "PERCENTAGE";};
				class KH_Time {name = "TIME"; value = "TIME";};
			};
		};
		class KH_ModuleAddActionDistance: Edit
		{
			displayName = "Distance";
			tooltip = "Distance within which the caller must be to the closest surface of the affected objects in order for the action to be usable. A caller moving beyond this distance will interrupt the action if it is currently in progress.";
			property = "KH_ModuleAddActionDistance";
			defaultValue = "'3'";
		};
		class KH_ModuleAddActionShowImmediately: Checkbox
		{
			displayName = "Show Immediately";
			tooltip = "True immediately displays the action upon it becoming usable. False requires the user to actively scroll through the action menu to see it.";
			property = "KH_ModuleAddActionShowImmediately";
			defaultValue = "true";
		};
		class KH_ModuleAddActionAllowIncapacitated: Checkbox
		{
			displayName = "Allow Incapacitated";
			tooltip = "True allows the action to be usable while the caller is incapacitated.";
			property = "KH_ModuleAddActionAllowIncapacitated";
			defaultValue = "false";
		};
		class KH_ModuleAddActionHideOnUse: Checkbox
		{
			displayName = "Hide On Use";
			tooltip = "True hides the action when it is used. This does not remove the action, but rather deselects it. This includes actions that have a duration.";
			property = "KH_ModuleAddActionHideOnUse";
			defaultValue = "false";
		};
		class KH_ModuleAddActionDrawHint: Checkbox
		{
			displayName = "Draw Hint";
			tooltip = "Displays a small interaction hint in 3D space that makes the presence of the action more clear to potential callers.";
			property = "KH_ModuleAddActionDrawHint";
			defaultValue = "false";
		};
		class KH_ModuleAddActionDetectionType: Combo
		{
			displayName = "Detection Type";
			tooltip = "Type of detection if Detection Show is not false. VIEW uses the caller's camera aim point. WEAPON uses the caller's weapon aim point. CURSOR uses the caller's cursor position on screen.";
			property = "KH_ModuleAddActionDetectionType";
			typeName = "STRING";
			defaultValue = "'VIEW'";
			class Values
			{
				class KH_Camera {name = "VIEW"; value = "VIEW";};
                class KH_Weapon {name = "WEAPON"; value = "WEAPON";};
                class KH_Cursor {name = "CURSOR"; value = "CURSOR";};
			};
		};
		class KH_ModuleAddActionDetectionShow: Edit
		{
			displayName = "Detection Show";
			tooltip = "Either a selection name on an affected object which must be observed in order for the action to be usable, or a lowercase boolean. In case of boolean, true makes it so that the caller must be looking at the surface of an affected object in order for the action to be usable, while false makes it so that the action is usable at any point for as long as the user is within the designated distance of an affected object.";
			property = "KH_ModuleAddActionDetectionShow";
			defaultValue = "'true'";
		};
		class KH_ModuleAddActionDetectionProgress: Edit
		{
			displayName = "Detection Progress";
			tooltip = "Either a selection name on an affected object which must be observed in order for the action to progress, or a lowercase boolean. In case of boolean, true makes it so that the caller must be looking at the surface of an affected object in order for the action to progress, while false makes it so that the action is usable at any point for as long as the user is within the designated distance of an affected object.";
			property = "KH_ModuleAddActionDetectionProgress";
			defaultValue = "'false'";
		};
		class KH_ModuleAddActionUserInput: Edit
		{
			displayName = "User Input";
			tooltip = "One of the key names defined in the config. This will bind corresponding keyboard key to this action. Leave empty for no custom key assignment.";
			property = "KH_ModuleAddActionUserInput";
			defaultValue = "''";
		};
		class KH_ModuleAddActionIdentifier: Edit
		{
			displayName = "Identifier";
			tooltip = "An identifier that is otherwise unique per action, but can be overriden by this attribute; if used, all actions tied to this identifier will share the same exclusivity and existence conditions. For example, if the action has a duration, exclusive is true, and repeatable is false, and two actions share the same identifier, then as soon as one of those actions is activated, the other one will not be usable and will be deleted when the first one completes. Action recovery rules and exist condition will be inherited from the first added action from the same action creator with this identifier. Leave empty to generate a unique identifier per action.";
			property = "KH_ModuleAddActionIdentifier";
			defaultValue = "''";
		};
		class KH_ModuleAddActionParent: Edit
		{
			displayName = "Parent";
			tooltip = "The variable name of a unit which makes it so that only the player controlling the unit with that variable name has the action available, or a lowercase boolean. In case of boolean, true makes it so that the action is available only if the player is currently controlling their player unit, while false makes it so that the action is available regardless of which unit the player is controlling.";
			property = "KH_ModuleAddActionParent";
			defaultValue = "'true'";
		};
		class KH_ModuleAddActionHandleParentActionRecovery: Checkbox
		{
			displayName = "Handle Parent Action Recovery";
			tooltip = "True recovers the action if a parent unit respawns, as well if the player controlled unit changes if the Parent Unit attribute being set to false.";
			property = "KH_ModuleAddActionHandleParentActionRecovery";
			defaultValue = "true";
		};
		class ModuleDescription: ModuleDescription {};
	};
	class ModuleDescription: ModuleDescription
	{
		description[] = {"Adds an action to the synchronized objects. If no objects are synchronized, the action is added to every player unit. The action addition happens once a potential caller comes within the Distance attribute's designated distance. Activates only once, without a trigger, on the server."};
		sync[] = {"Anything"};
		position = 0;
		direction = 0;
		optional = 0;
		duplicate = 1;
	};
};