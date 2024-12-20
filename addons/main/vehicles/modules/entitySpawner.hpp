class KH_ModuleEntitySpawner: Module_F
{
	scope = 2;
	scopeCurator = 1;
	canSetArea = 0;
	canSetAreaHeight = 0;
	canSetAreaShape = 0;
	displayName = "Entity Spawner";
	category = "KH_Modules";
	function = "KH_fnc_moduleEntitySpawner";
	functionPriority = 1;
	isGlobal = 0;
	isTriggerActivated = 0;
	isDisposable = 1;
	is3DEN = 0;
	icon = "\a3\missions_f_curator\data\img\iconmptypesectorcontrol_ca.paa";
	curatorInfoType = "";
	curatorInfoTypeEmpty = "";
	curatorCanAttach = 0;
	class Attributes: AttributesBase
	{
		class KH_ModuleEntitySpawnerEntityTypes: Edit
		{
			displayName = "Entity Types";
			tooltip = "Array of strings of class names of entities that will be spawned by this spawner.";
			property = "KH_ModuleEntitySpawnerEntityTypes";
			defaultValue = "'[]'";
		};
		class KH_ModuleEntitySpawnerRadius: Edit
		{
			displayName = "Radius";
			tooltip = "Array of scalars dictating the 3D radius within which an entity can be spawned around the synchronized entities, in format <[x, y, z]>.";
			property = "KH_ModuleEntitySpawnerRadius";
			defaultValue = "'[]'";
		};
		class KH_ModuleEntitySpawnerAmount: Edit
		{
			displayName = "Amount";
			tooltip = "Amount of entities to spawn per spawn interval.";
			property = "KH_ModuleEntitySpawnerAmount";
			defaultValue = "''";
		};
		class KH_ModuleEntitySpawnerMaximum: Edit
		{
			displayName = "Maximum";
			tooltip = "Maximum amount of entities that can be spawned by this spawner. Each spawned entity is tracked, and is subtracted upon death or deletion. The number of spawned entities may exceed <Maximum> if the amount of entities is below <Maximum> at the time of a spawn interval, that is only if <Amount> dictates a number that would exceed <Maximum> at that time.";
			property = "KH_ModuleEntitySpawnerMaximum";
			defaultValue = "''";
		};
		class KH_ModuleEntitySpawnerCondition: Edit
		{
			displayName = "Condition";
			tooltip = "Unscheduled code, executed on the server, that must return <true> in order to allow for the spawner to spawn entities. Passed arguments available through <_this> are: <[_transforms (ARRAY), _spawnerCount (SCALAR), _handlerId (ARRAY)]>.";
			property = "KH_ModuleEntitySpawnerCondition";
			defaultValue = "''";
		};
		class KH_ModuleEntitySpawnerInitialization: Edit
		{
			displayName = "Initialization";
			tooltip = "Unscheduled code to execute on the server for each entity that spawns. Passed arguments available through <_this> are: <[_entity (OBJECT), _chosenTransforms (ARRAY), _position (ARRAY)]>.";
			property = "KH_ModuleEntitySpawnerInitialization";
			defaultValue = "''";
		};
		class KH_ModuleEntitySpawnerType: Edit
		{
			displayName = "Type";
			tooltip = "Specific type of the entities that will spawn. Possible types are: <['UNIT', _placementMode (STRING), _side (SIDE), _shareGroup (BOOLEAN)]>, <['AGENT', _placementMode (STRING)]>, <['OBJECT', _placementMode (STRING), _local (BOOLEAN)]>, <['SIMPLE_OBJECT', _local (BOOLEAN)]>, <['VEHICLE', _placementMode (STRING)]>.";
			property = "KH_ModuleEntitySpawnerType";
			defaultValue = "'[]'";
		};
		class KH_ModuleEntitySpawnerInterval: Edit
		{
			displayName = "Interval";
			tooltip = "Interval at which the spawner will attempt to spawn entities, in seconds";
			property = "KH_ModuleEntitySpawnerInterval";
			defaultValue = "''";
		};
		class ModuleDescription: ModuleDescription {};
	};
	class ModuleDescription: ModuleDescription
	{
		description[] = {"Periodically spawns entities at or around the synchronized entities. Does not need a trigger to activate. Can activate only once. Activated on the server."};
		sync[] = {"AnyStaticObject", "AnyVehicle", "AnyPerson"};
		position = 0;
		direction = 0;
		optional = 0;
		duplicate = 1;
	};
};