class KH_ModuleEquipableObject: Module_F
{
	author = "Kharos";
	scope = 2;
	scopeCurator = 1;
	canSetArea = 0;
	canSetAreaHeight = 0;
	canSetAreaShape = 0;
	displayName = "Equipable Object";
	category = "KH_Modules";
	function = "KH_fnc_moduleEquipableObject";
	functionPriority = 1;
	isGlobal = 0;
	isTriggerActivated = 0;
	isDisposable = 1;
	is3DEN = 0;
	icon = "\a3\Ui_f\data\IGUI\Cfg\simpleTasks\types\box_ca.paa";
	class Attributes: AttributesBase
	{
		class Name: Edit
		{
			displayName = "Name";
			tooltip = "Name assigned to the object for equip and unequip actions.";
			property = "KH_ModuleEquipableObjectName";
			defaultValue = "''";
		};
		class Bone: Edit
		{
			displayName = "Bone";
			tooltip = "Name of the bone on the equipping unit's skeleton to which the object will be attached. Set to an empty string to attach at the root.";
			property = "KH_ModuleEquipableObjectBone";
			defaultValue = "''";
		};
		class Position: Edit
		{
			displayName = "Position";
			tooltip = "Position of the object relative to the assigned bone.";
			property = "KH_ModuleEquipableObjectPosition";
			defaultValue = "'0, 0, 0'";
		};
		class Rotation: Edit
		{
			displayName = "Rotation";
			tooltip = "Rotation of the object relative to the assigned bone. Can either be in euler format [x, y, z], or vectorDir and vectorUp format [x, y, z], [x, y, z]";
			property = "KH_ModuleEquipableObjectRotation";
			defaultValue = "'0, 0, 0'";
		};
		class Scale: Edit
		{
			displayName = "Scale";
			tooltip = "Scale of the object while it is equipped.";
			property = "KH_ModuleEquipableObjectScale";
			defaultValue = "'1'";
		};
		class DisableCollision: Checkbox
		{
			displayName = "Disable Collision";
			tooltip = "True disables collision on this object while it is equipped.";
			property = "KH_ModuleEquipableObjectDisableCollision";
			defaultValue = "true";
		};
		class HideInVehicles: Checkbox
		{
			displayName = "Hide In Vehicles";
			tooltip = "True hides the object while equipped on a unit that is inside of a vehicle.";
			property = "KH_ModuleEquipableObjectHideInVehicles";
			defaultValue = "true";
		};
		class ToggleEquip: Checkbox
		{
			displayName = "Toggle Equip";
			tooltip = "True allows the object to be unequipped and reequipped repeatedly.";
			property = "KH_ModuleEquipableObjectToggleEquip";
			defaultValue = "true";
		};
		class Exclusive: Checkbox
		{
			displayName = "Exclusive";
			tooltip = "True prevents a unit from equipping another object with this same exclusive flag set to true while this object is equipped.";
			property = "KH_ModuleEquipableObjectExclusive";
			defaultValue = "true";
		};
		class ModuleDescription: ModuleDescription {};
	};
	class ModuleDescription: ModuleDescription
	{
		description[] = {"Adds an action to the synchronized objects that allows players to equip them on their person. Activates only once, without a trigger, on the server."};
		sync[] = {"AnyStaticObject"};
		position = 0;
		direction = 0;
		optional = 0;
		duplicate = 0;
	};
};