class KH_EngineeringSettings: Title
{
	attributeLoad = "[_this controlsGroupCtrl 103, _this controlsGroupCtrl 104, 'x'] call BIS_fnc_initSliderValue; [_this controlsGroupCtrl 103, _this controlsGroupCtrl 104, 'x', _value select 3] call BIS_fnc_initSliderValue; [_this controlsGroupCtrl 105, _this controlsGroupCtrl 106, 'x'] call BIS_fnc_initSliderValue; [_this controlsGroupCtrl 105, _this controlsGroupCtrl 106, 'x', _value select 3] call BIS_fnc_initSliderValue; [_this controlsGroupCtrl 107, _this controlsGroupCtrl 108, 'x'] call BIS_fnc_initSliderValue; [_this controlsGroupCtrl 107, _this controlsGroupCtrl 108, 'x', _value select 3] call BIS_fnc_initSliderValue; [_this, _value] call KH_fnc_loadControlAttributes;";
	attributeSave = "[_this] call KH_fnc_saveControlAttributes;";
	h = QUOTE(7 * CTRL_DEFAULT_H + 80 * pixelH);
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
		class KHEngineeringHandlingTitle: Title
		{
			text = "KH Engineering Handling";
			tooltip = "True enables the KH Engineering system on this unit.";
			y = QUOTE(2 * CTRL_DEFAULT_H + 5 * CTRL_DEFAULT_Y);
		};
		class KHEngineeringHandling: ctrlCheckbox
		{
			idc = 101;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(2 * CTRL_DEFAULT_H + 5 * CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class RecoverableTitle: Title
		{
			text = "Recoverable";
			tooltip = "True prevents the vehicle from ever dying; the vehicle can only ever be disabled.";
			y = QUOTE(3 * CTRL_DEFAULT_H + 10 * CTRL_DEFAULT_Y);
		};
		class Recoverable: ctrlCheckbox
		{
			idc = 102;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(3 * CTRL_DEFAULT_H + 10 * CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class DamageMultiplierTitle: Title
		{
			text = "Damage Multiplier";
			tooltip = "All damage calculations for hit points and total vehicle health are multiplied by this value.";
			y = QUOTE(4 * CTRL_DEFAULT_H + 15 * CTRL_DEFAULT_Y);
		};
		class DamageMultiplier: ctrlXSliderH
		{
			idc = 103;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(4 * CTRL_DEFAULT_H + 15 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W * 0.85);
			h = QUOTE(CTRL_DEFAULT_H);
        	sliderRange[] = {0, 10};
       		sliderStep = 0.01;
        	lineSize = 1;
		};
		class DamageMultiplierEdit: ctrlEdit
		{
			idc = 104;
			x = QUOTE(CTRL_DEFAULT_X * 2.455);
			y = QUOTE(4 * CTRL_DEFAULT_H + 15 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W * 0.15);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class TotalDamageMultiplierTitle: Title
		{
			text = "Total Damage Multiplier";
			tooltip = "All damage calculations for the total vehicle health are multiplied by this value.";
			y = QUOTE(5 * CTRL_DEFAULT_H + 20 * CTRL_DEFAULT_Y);
		};
		class TotalDamageMultiplier: ctrlXSliderH
		{
			idc = 105;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(5 * CTRL_DEFAULT_H + 20 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W * 0.85);
			h = QUOTE(CTRL_DEFAULT_H);
        	sliderRange[] = {0, 10};
       		sliderStep = 0.01;
        	lineSize = 1;
		};
		class TotalDamageMultiplierEdit: ctrlEdit
		{
			idc = 106;
			x = QUOTE(CTRL_DEFAULT_X * 2.455);
			y = QUOTE(5 * CTRL_DEFAULT_H + 20 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W * 0.15);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class ImpactDamageMultiplierTitle: Title
		{
			text = "Impact Damage Multiplier";
			tooltip = "All impact damage calculations are multiplied by this value.";
			y = QUOTE(6 * CTRL_DEFAULT_H + 25 * CTRL_DEFAULT_Y);
		};
		class ImpactDamageMultiplier: ctrlXSliderH
		{
			idc = 107;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(6 * CTRL_DEFAULT_H + 25 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W * 0.85);
			h = QUOTE(CTRL_DEFAULT_H);
        	sliderRange[] = {0, 10};
       		sliderStep = 0.01;
        	lineSize = 1;
		};
		class ImpactDamageMultiplierEdit: ctrlEdit
		{
			idc = 108;
			x = QUOTE(CTRL_DEFAULT_X * 2.455);
			y = QUOTE(6 * CTRL_DEFAULT_H + 25 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W * 0.15);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class HitPointDamageMultipliersTitle: Title
		{
			text = "Hit Point Damage Multipliers";
			tooltip = "Hashmap style arrays where the key is the name of a hit point, and the value is the multiplier for any damage received to that hit point.";
			y = QUOTE(7 * CTRL_DEFAULT_H + 30 * CTRL_DEFAULT_Y);
		};
		class HitPointDamageMultipliers: ctrlEdit
		{
			idc = 109;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(7 * CTRL_DEFAULT_H + 30 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
	};
};