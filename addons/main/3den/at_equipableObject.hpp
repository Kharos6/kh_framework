class KH_EquipableObject: Title
{
	attributeLoad = "[_this, _value] call KH_fnc_loadControlAttributes;";
	attributeSave = "[_this] call KH_fnc_saveControlAttributes;";
	h = QUOTE(12 * CTRL_DEFAULT_H + 70 * pixelH);
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
		class KH_BoneTitle: Title
		{
			text = "Bone";
			tooltip = "Bone to which the object will be attached. If empty, the object will be attached to the origin of the unit.";
			y = QUOTE(2 * CTRL_DEFAULT_H + 5 * CTRL_DEFAULT_Y);
		};
		class KH_Bone: ctrlEdit
		{
			idc = 101;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(2 * CTRL_DEFAULT_H + 5 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_PositionTitle: Title
		{
			text = "Position";
			tooltip = "Relative position array, relative to the bone, where the object will be attached, in format: <[x, y, z]>.";
			y = QUOTE(3 * CTRL_DEFAULT_H + 10 * CTRL_DEFAULT_Y);
		};
		class KH_Position: ctrlEdit
		{
			idc = 102;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(3 * CTRL_DEFAULT_H + 10 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_RotationTitle: Title
		{
			text = "Rotation";
			tooltip = "Radial rotation array, relative to the bone, at which the object will be attached, in format: <[x, y, z]>.";
			y = QUOTE(4 * CTRL_DEFAULT_H + 15 * CTRL_DEFAULT_Y);
		};
		class KH_Rotation: ctrlEdit
		{
			idc = 103;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(4 * CTRL_DEFAULT_H + 15 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_ScaleTitle: Title
		{
			text = "Scale";
			tooltip = "Scale of the object after being equipped.";
			y = QUOTE(5 * CTRL_DEFAULT_H + 20 * CTRL_DEFAULT_Y);
		};
		class KH_Scale: ctrlEdit
		{
			idc = 104;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(5 * CTRL_DEFAULT_H + 20 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_MassTitle: Title
		{
			text = "Mass";
			tooltip = "Mass of the object after being equipped. Default value of <1> advised for preventing physics collisions.";
			y = QUOTE(6 * CTRL_DEFAULT_H + 25 * CTRL_DEFAULT_Y);
		};
		class KH_Mass: ctrlEdit
		{
			idc = 105;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(6 * CTRL_DEFAULT_H + 25 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_HideInVehiclesTitle: Title
		{
			text = "Hide In Vehicles";
			tooltip = "<true> hides the object when the carrying unit enters a vehicle. Not using this option risks the object destroying vehicles moving at high speeds.";
			y = QUOTE(7 * CTRL_DEFAULT_H + 30 * CTRL_DEFAULT_Y);
		};
		class KH_HideInVehicles: ctrlCheckbox
		{
			idc = 106;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(7 * CTRL_DEFAULT_H + 30 * CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_ToggleEquipTitle: Title
		{
			text = "Toggle Equip";
			tooltip = "<true> allows this object to be unequipped and reequipped.";
			y = QUOTE(8 * CTRL_DEFAULT_H + 35 * CTRL_DEFAULT_Y);
		};
		class KH_ToggleEquip: ctrlCheckbox
		{
			idc = 107;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(8 * CTRL_DEFAULT_H + 35 * CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_ExclusiveTitle: Title
		{
			text = "Exclusive";
			tooltip = "<true> makes it so that a player cannot equip another object if that other object also has this <Exclusive> option set to <true>.";
			y = QUOTE(9 * CTRL_DEFAULT_H + 40 * CTRL_DEFAULT_Y);
		};
		class KH_Exclusive: ctrlCheckbox
		{
			idc = 108;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(9 * CTRL_DEFAULT_H + 40 * CTRL_DEFAULT_Y);
			w = QUOTE(5 * GRID_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_EventTitle: Title
		{
			text = "Event";
			tooltip = "Event to raise when this object is equipped and unequipped. Event <'KH_eve_objectEquipped'> is used if empty. Passed arguments available through <_this> are: <[_unit (OBJECT), _object (OBJECT), _isEquipped (BOOLEAN)]>.";
			y = QUOTE(10 * CTRL_DEFAULT_H + 45 * CTRL_DEFAULT_Y);
		};
		class KH_Event: ctrlEdit
		{
			idc = 109;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(10 * CTRL_DEFAULT_H + 45 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
		class KH_ObjectNameTitle: Title
		{
			text = "Object Name";
			tooltip = "Name to display in the action prompt for equipping and unequipping this object.";
			y = QUOTE(11 * CTRL_DEFAULT_H + 50 * CTRL_DEFAULT_Y);
		};
		class KH_ObjectName: ctrlEdit
		{
			idc = 110;
			x = QUOTE(CTRL_DEFAULT_X);
			y = QUOTE(11 * CTRL_DEFAULT_H + 50 * CTRL_DEFAULT_Y);
			w = QUOTE(CTRL_DEFAULT_W);
			h = QUOTE(CTRL_DEFAULT_H);
		};
	};
};