class KH_ParticleEmitterDropdown: Combo
{
	attributeLoad = "_array = (('true' configClasses (configFile >> 'CfgCloudlets')) + (('true' configClasses configFile) select {(isText (_x >> 'simulation')) && (isText (_x >> 'type')) && (isNumber (_x >> 'intensity')) && (isNumber (_x >> 'lifeTime')) && (isNumber (_x >> 'interval')) && (isArray (_x >> 'position'));})); _array sort true; {(_this controlsGroupCtrl 100) lbAdd (configName _x)} forEach _array; (_this controlsGroupCtrl 100) lbSetCurSel (_value select 1);";
	attributeSave = "[(_this controlsGroupCtrl 100) lbText (lbCurSel (_this controlsGroupCtrl 100)), lbCurSel (_this controlsGroupCtrl 100)];";
    onLoad = "_ctrlGroup = _this select 0; {_ctrlGroup lbAdd (configName _x)} forEach (('true' configClasses (configFile >> 'CfgCloudlets')) + (('true' configClasses configFile) select {(isText (_x >> 'simulation')) && (isText (_x >> 'type')) && (isNumber (_x >> 'intensity')) && (isNumber (_x >> 'lifeTime')) && (isNumber (_x >> 'interval')) && (isArray (_x >> 'position'));}));";
	h = QUOTE(CTRL_DEFAULT_H + 2 * pixelH);
	class Controls: Controls
	{
		class Title: Title
		{
			text = "Particle";
			tooltip = "Particle to be used, can be a cloudlet class or a base class.";
			y = QUOTE(CTRL_DEFAULT_H * CTRL_DEFAULT_Y);
		};
		class Value: ctrlCombo
		{
			idc = 100;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(CTRL_DEFAULT_H * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
	};
};