class KH_SetCameraTexture: Title
{
	attributeLoad = "(_this controlsGroupCtrl 104) lbAdd '1'; (_this controlsGroupCtrl 104) lbAdd '2'; (_this controlsGroupCtrl 104) lbAdd '3'; (_this controlsGroupCtrl 104) lbAdd '4'; (_this controlsGroupCtrl 104) lbAdd '5'; (_this controlsGroupCtrl 104) lbAdd '6'; (_this controlsGroupCtrl 104) lbAdd '7'; (_this controlsGroupCtrl 104) lbAdd '8'; [_this, _value] call KH_fnc_loadControlAttributes;";
	attributeSave = "[_this] call KH_fnc_saveControlAttributes;";
	h = QUOTE(6 * CTRL_DEFAULT_H + 70 * pixelH);
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
		class KH_PositionEntityTitle: Title
		{
			text = "Position Entity";
			tooltip = "Variable name of the entity to which the camera will be attached.";
			y = QUOTE(2 * CTRL_DEFAULT_H + 5 * CTRL_DEFAULT_Y);
		};
		class KH_PositionEntity: ctrlEdit
		{
			idc = 101;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(2 * CTRL_DEFAULT_H + 5 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_TargetEntityTitle: Title
		{
			text = "Target Entity";
			tooltip = "Variable name of the entity which the camera will look at.";
			y = QUOTE(3 * CTRL_DEFAULT_H + 10 * CTRL_DEFAULT_Y);
		};
		class KH_TargetEntity: ctrlEdit
		{
			idc = 102;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(3 * CTRL_DEFAULT_H + 10 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_TextureTitle: Title
		{
			text = "Texture";
			tooltip = "Texture index to which the camera view will be applied.";
			y = QUOTE(4 * CTRL_DEFAULT_H + 15 * CTRL_DEFAULT_Y);
		};
		class KH_Texture: ctrlEdit
		{
			idc = 103;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(4 * CTRL_DEFAULT_H + 15 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_RenderTargetTitle: Title
		{
			text = "Render Target";
			tooltip = "Render target to assign to the camera. Multiple cameras cannot share the same render target.";
			y = QUOTE(5 * CTRL_DEFAULT_H + 20 * CTRL_DEFAULT_Y);
		};
		class KH_RenderTarget: ctrlCombo
		{
			idc = 104;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(5 * CTRL_DEFAULT_H + 20 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
	};
};