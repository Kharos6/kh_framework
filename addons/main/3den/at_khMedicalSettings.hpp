class KH_MedicalSettings: Title
{
	attributeLoad = "[_this controlsGroupCtrl 104, _this controlsGroupCtrl 105, ''] call BIS_fnc_initSliderValue; [_this controlsGroupCtrl 104, _this controlsGroupCtrl 105, '', _value select 3] call BIS_fnc_initSliderValue; [_this, _value] call KH_fnc_loadControlAttributes;";
	attributeSave = "[_this] call KH_fnc_saveControlAttributes;";
	h = QUOTE(6 * CTRL_DEFAULT_H + 70 * pixelH);
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
		class KHMedicalHandlingTitle: Title
		{
			text = "KH Medical Handling";
			tooltip = "False disables the KH Medical system on this unit, even if the global KH medical system is enabled.";
			y = QUOTE(2 * CTRL_DEFAULT_H + CTRL_DEFAULT_Y);
		};
		class KHMedicalHandling: ctrlCheckbox
		{
			idc = 101;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(2 * CTRL_DEFAULT_H + CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class GlobalDamageMultipliersTitle: Title
		{
			text = "Global Damage Multipliers";
			tooltip = "False ignores the global overall damage multiplication for this unit, only allowing the overall damage multiplication from the Damage Multiplier attribute for this unit. True combines the Damage Bultiplier attribute for this unit with the global one.";
			y = QUOTE(3 * CTRL_DEFAULT_H + CTRL_DEFAULT_Y);
		};
		class GlobalDamageMultipliers: ctrlCheckbox
		{
			idc = 102;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(3 * CTRL_DEFAULT_H + CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class PlotArmorTitle: Title
		{
			text = "Plot Armor";
			tooltip = "True prevents the unit from ever dying; mortal wounds will only result in unconsciousness.";
			y = QUOTE(4 * CTRL_DEFAULT_H + CTRL_DEFAULT_Y);
		};
		class PlotArmor: ctrlCheckbox
		{
			idc = 103;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(4 * CTRL_DEFAULT_H + CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class DamageMultiplierTitle: Title
		{
			text = "Damage Multiplier";
			tooltip = "All damage calculations for all hit points and total unit health are multiplied by this value.";
			y = QUOTE(5 * CTRL_DEFAULT_H + 20 * CTRL_DEFAULT_Y);
		};
		class DamageMultiplier: ctrlXSliderH
		{
			idc = 104;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(5 * CTRL_DEFAULT_H + 20 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W * 0.85);
			h = QUOTE(CTRL_DEFAULT_H);
        	sliderRange[] = {0, 10};
       		sliderStep = 0.01;
        	lineSize = 1;
		};
		class DamageMultiplierEdit: ctrlEdit
		{
			idc = 105;
			x = QUOTE(CTRL_DEFAULT_X * 2.455);
			y = QUOTE(5 * CTRL_DEFAULT_H + 20 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W * 0.15);
			h = QUOTE(CTRL_DEFAULT_H);
		};
	};
};