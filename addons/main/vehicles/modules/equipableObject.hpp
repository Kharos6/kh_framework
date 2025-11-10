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
	curatorInfoType = "";
	curatorInfoTypeEmpty = "";
	curatorCanAttach = 0;
	icon = "\a3\Ui_f\data\IGUI\Cfg\simpleTasks\types\box_ca.paa";
	class Attributes: AttributesBase
	{
		class KH_ModuleEquipableObjectName: Edit
		{
			displayName = "Name";
			tooltip = "Name assigned to the object for equip and unequip actions. Leave empty to assign name based on the displayName of the object's config.";
			property = "KH_ModuleEquipableObjectName";
			defaultValue = "''";
		};
		class KH_ModuleEquipableObjectBone: Edit
		{
			displayName = "Bone";
			tooltip = "Name of the bone on the equipping unit's skeleton to which the object will be attached. Leave empty to attach at the root.";
			property = "KH_ModuleEquipableObjectBone";
			defaultValue = "''";
		};
		class KH_ModuleEquipableObjectPosition: EditXYZ
		{
			displayName = "Position";
			tooltip = "Position of the object relative to the assigned bone.";
			property = "KH_ModuleEquipableObjectPosition";
			defaultValue = "[0, 0, 0]";
		};
		class KH_ModuleEquipableObjectRotation: EditXYZ
		{
			displayName = "Rotation";
			tooltip = "Rotation of the object relative to the assigned bone.";
			property = "KH_ModuleEquipableObjectRotation";
			defaultValue = "[0, 0, 0]";
		};
		class KH_ModuleEquipableObjectScale: Edit
		{
			displayName = "Scale";
			tooltip = "Scale of the object while it is equipped.";
			property = "KH_ModuleEquipableObjectScale";
			defaultValue = "'1'";
		};
		class KH_ModuleEquipableObjectDisableCollision: Checkbox
		{
			displayName = "Disable Collision";
			tooltip = "True disables collision on this object while it is equipped.";
			property = "KH_ModuleEquipableObjectDisableCollision";
			defaultValue = "true";
		};
		class KH_ModuleEquipableObjectHideInVehicles: Checkbox
		{
			displayName = "Hide In Vehicles";
			tooltip = "True hides the object while equipped on a unit that is inside of a vehicle.";
			property = "KH_ModuleEquipableObjectHideInVehicles";
			defaultValue = "true";
		};
		class KH_ModuleEquipableObjectExclusive: Checkbox
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
		description[] = {"Adds an action to the synchronized entities that allows players to equip them on their person. Each equip and unequip triggers the 'KH_eve_equipableObjectExchanged' event, where passed arguments available through _this are: [_carrier (OBJECT), _equipableObject (OBJECT), _equipped (BOOL)]. Activates only once, without a trigger, on the server."};
		sync[] = {"AnyStaticObject"};
		position = 0;
		direction = 0;
		optional = 0;
		duplicate = 1;
	};
};