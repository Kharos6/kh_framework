class KH_VisualTrigger: Title
{
	attributeLoad = "[_this controlsGroupCtrl 101, _this controlsGroupCtrl 111, '%'] call BIS_fnc_initSliderValue; [_this controlsGroupCtrl 101, _this controlsGroupCtrl 111, '%', _value select 1] call BIS_fnc_initSliderValue; [_this, _value] call KH_fnc_loadControlAttributes;";
	attributeSave = "[_this] call KH_fnc_saveControlAttributes;";
	h = QUOTE(20 * CTRL_DEFAULT_H + 70 * pixelH);
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
		class KH_ScreenMultiplierTitle: Title
		{
			text = "Screen Multiplier";
			tooltip = "Percentage of a player's screen, calculated from the center of the screen, within which the entity's center has to be in order to activate the trigger. For example, if the value is 100%, the trigger will activate as soon as the entity enters the screen.";
			y = QUOTE(2 * CTRL_DEFAULT_H + 5 * CTRL_DEFAULT_Y);
		};
		class KH_ScreenMultiplier: ctrlXSliderH
		{
			idc = 101;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(2 * CTRL_DEFAULT_H + 5 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W * 0.85);
			h = QUOTE(CTRL_DEFAULT_H);
        	sliderRange[] = {0, 1};
       		sliderStep = 0.01;
        	lineSize = 1;
		};
		class KH_ProximityTitle: Title
		{
			text = "Proximity";
			tooltip = "Distance from a player to the entity, within which the trigger will activate regardless of the entity's presence on a player's screen.";
			y = QUOTE(3 * CTRL_DEFAULT_H + 10 * CTRL_DEFAULT_Y);
		};
		class KH_Proximity: ctrlEdit
		{
			idc = 102;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(3 * CTRL_DEFAULT_H + 10 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_DistanceTitle: Title
		{
			text = "Distance";
			tooltip = "Distance from a player to the entity, within which the trigger is allowed to activate when the entity is present on a player's screen.";
			y = QUOTE(4 * CTRL_DEFAULT_H + 15 * CTRL_DEFAULT_Y);
		};
		class KH_Distance: ctrlEdit
		{
			idc = 103;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(4 * CTRL_DEFAULT_H + 15 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_ConditionServerTitle: Title
		{
			text = "Server Condition";
			tooltip = "Unscheduled code, executed on the server, which must return <true> in order to allow the trigger to activate.";
			y = QUOTE(5 * CTRL_DEFAULT_H + 20 * CTRL_DEFAULT_Y);
		};
		class KH_ConditionServer: ctrlEditMulti
		{
			idc = 104;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(5 * CTRL_DEFAULT_H + 20 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H * 3);
		};
		class KH_ConditionPlayerTitle: Title
		{
			text = "Player Condition";
			tooltip = "Unscheduled code, executed locally to each player, which must return <true> in order to allow them to activate the trigger.";
			y = QUOTE(8 * CTRL_DEFAULT_H + 25 * CTRL_DEFAULT_Y);
		};
		class KH_ConditionPlayer: ctrlEditMulti
		{
			idc = 105;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(8 * CTRL_DEFAULT_H + 25 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H * 3);
		};
		class KH_TrueFunctionTitle: Title
		{
			text = "True Function";
			tooltip = "Unscheduled code to execute on the server when the trigger is activated. Passed arguments available through <_this> are: [_player, _entity, _triggerId].";
			y = QUOTE(11 * CTRL_DEFAULT_H + 30 * CTRL_DEFAULT_Y);
		};
		class KH_TrueFunction: ctrlEditMulti
		{
			idc = 106;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(11 * CTRL_DEFAULT_H + 30 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H * 3);
		};
		class KH_FalseFunctionTitle: Title
		{
			text = "False Function";
			tooltip = "Unscheduled code to execute on the server when the trigger is deactivated. Passed arguments available through <_this> are: [_player, _entity, _triggerId].";
			y = QUOTE(14 * CTRL_DEFAULT_H + 35 * CTRL_DEFAULT_Y);
		};
		class KH_FalseFunction: ctrlEditMulti
		{
			idc = 107;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(14 * CTRL_DEFAULT_H + 35 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H * 3);
		};
		class KH_RepeatableTitle: Title
		{
			text = "Repeatable";
			tooltip = "<true> allows the trigger to activate and deactivate multiple times. <false> only allows the trigger to activate and deactivate only once.";
			y = QUOTE(17 * CTRL_DEFAULT_H + 40 * CTRL_DEFAULT_Y);
		};
		class KH_Repeatable: ctrlCheckbox
		{
			idc = 108;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(17 * CTRL_DEFAULT_H + 40 * CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_IntervalTitle: Title
		{
			text = "Interval";
			tooltip = "Interval at which the trigger state is checked, in seconds.";
			y = QUOTE(18 * CTRL_DEFAULT_H + 45 * CTRL_DEFAULT_Y);
		};
		class KH_Interval: ctrlEdit
		{
			idc = 109;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(18 * CTRL_DEFAULT_H + 45 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_SharedTitle: Title
		{
			text = "Shared";
			tooltip = "<true> makes it so that if a player activates the trigger, it will not deactivate for as long as any player maintains line of sight to the entity. <false> makes it so that the trigger activates and deactivates separately for each player based on the entity's presence on their screen.";
			y = QUOTE(19 * CTRL_DEFAULT_H + 50 * CTRL_DEFAULT_Y);
		};
		class KH_Shared: ctrlCheckbox
		{
			idc = 110;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(19 * CTRL_DEFAULT_H + 50 * CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_ScreenMultiplierEdit: ctrlEdit
		{
			idc = 111;
			x = QUOTE(CTRL_DEFAULT_X * 2.455);
			y = QUOTE(2 * CTRL_DEFAULT_H + 5 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W * 0.15);
			h = QUOTE(CTRL_DEFAULT_H);
		};
	};
};