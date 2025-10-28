class KH_ModuleEntitySpawner: Module_F
{
	author = "Kharos";
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
	isTriggerActivated = 1;
	isDisposable = 0;
	is3DEN = 0;
	curatorInfoType = "";
	curatorInfoTypeEmpty = "";
	curatorCanAttach = 0;
	icon = "\a3\Missions_F_Curator\data\img\iconMPTypeSectorControl_ca.paa";
	class Attributes: AttributesBase
	{
		class KH_ModuleEntitySpawnerEntityTypes: Edit
		{
			displayName = "Entity Types";
			tooltip = "Strings of class names of entity types from CfgVehicles that will be chosen at random to spawn. In format 'entity1', 'entity2', 'entity3', ....";
			property = "KH_ModuleEntitySpawnerEntityTypes";
			defaultValue = "''";
		};
		class KH_ModuleEntitySpawnerType: Combo
		{
			displayName = "Type";
			tooltip = "Type of entities to spawn.";
			property = "KH_ModuleEntitySpawnerType";
			typeName = "STRING";
			defaultValue = "'UNIT'";
			class Values
			{
				class KH_Unit {name = "UNIT"; value = "UNIT";};
                class KH_Agent {name = "AGENT"; value = "AGENT";};
                class KH_Group {name = "GROUP"; value = "GROUP";};
                class KH_Object {name = "OBJECT"; value = "OBJECT";};
                class KH_SimpleObject {name = "SIMPLE OBJECT"; value = "SIMPLE_OBJECT";};
                class KH_Vehicle {name = "VEHICLE"; value = "VEHICLE";};
			};
		};
		class KH_ModuleEntitySpawnerPlacementMode: Combo
		{
			displayName = "Placement Mode";
			tooltip = "Placement mode. FLY will have the same effect as NONE for agents or units.";
			property = "KH_ModuleEntitySpawnerPlacementMode";
			typeName = "STRING";
			defaultValue = "'CAN_COLLIDE'";
			class Values
			{
				class KH_NONE {name = "NONE"; value = "NONE";};
                class KH_CanCollide {name = "CAN COLLIDE"; value = "CAN_COLLIDE";};
                class KH_Fly {name = "FLY"; value = "FLY";};
			};
		};
		class KH_ModuleEntitySpawnerSide: Combo
		{
			displayName = "Side";
			tooltip = "Entity side. Has no effect for VEHICLE, OBJECT, and SIMPLE OBJECT";
			property = "KH_ModuleEntitySpawnerSide";
			typeName = "STRING";
			defaultValue = "'BLUFOR'";
			class Values
			{
				class KH_West {name = "BLUFOR"; value = "BLUFOR";};
                class KH_East {name = "OPFOR"; value = "OPFOR";};
                class KH_Resistance {name = "GREENFOR"; value = "GREENFOR";};
                class KH_Civilian {name = "CIVILIAN"; value = "CIVILIAN";};
			};
		};
		class KH_ModuleEntitySpawnerRadius: Edit
		{
			displayName = "Radius";
			tooltip = "Randomly chosen spawn position within a 3d vector radius, in format x, y, z.";
			property = "KH_ModuleEntitySpawnerRadius";
			defaultValue = "'100, 100, 0'";
		};
		class KH_ModuleEntitySpawnerAmount: Edit
		{
			displayName = "Amount";
			tooltip = "Amount the spawned entities per interval. For GROUP type, this does not mean the number of groups, but rather the amount of units within the group.";
			property = "KH_ModuleEntitySpawnerAmount";
			defaultValue = "'1'";
		};
		class KH_ModuleEntitySpawnerMaximum: Edit
		{
			displayName = "Maximum";
			tooltip = "Maximum allowed amount the spawned entities per interval.";
			property = "KH_ModuleEntitySpawnerMaximum";
			defaultValue = "'1'";
		};
		class KH_ModuleEntitySpawnerCondition: EditCodeMulti5
		{
			displayName = "Condition";
			tooltip = "Unscheduled code executed locally to the server that must return true in order for a spawn to be allowed. Passed arguments available through _this are: [_chosenSpawnPoint (ARRAY), _spawnedCount (SCALAR)]. Local variables exposed to the function are: _handlerId.";
			property = "KH_ModuleEntitySpawnerCondition";
			defaultValue = "'true'";
		};
		class KH_ModuleEntitySpawnerInit: EditCodeMulti5
		{
			displayName = "Init";
			tooltip = "Unscheduled code executed locally to the server acting as the initialization of each spawned entity. Passed arguments available through _this are: [_entity (OBJECT), _chosenSpawnPoint (ARRAY), _chosenSpawnPosition (ARRAY)]. Local variables exposed to the function are: _handlerId.";
			property = "KH_ModuleEntitySpawnerInit";
			defaultValue = "''";
		};
		class KH_ModuleEntitySpawnerInterval: Edit
		{
			displayName = "Interval";
			tooltip = "Spawn interval.";
			property = "KH_ModuleEntitySpawnerInterval";
			defaultValue = "'1'";
		};
		class KH_ModuleEntitySpawnerCountKilled: Checkbox
		{
			displayName = "Count Killed";
			tooltip = "True makes it so that killed entities decrement the spawned count, allowing for others to take its place.";
			property = "KH_ModuleEntitySpawnerCountKilled";
			defaultValue = "true";
		};
		class KH_ModuleEntitySpawnerValidatePosition: Checkbox
		{
			displayName = "Validate Position";
			tooltip = "True makes it so that the spawner refuses to spawn an entity if a player is looking at the chosen spawn position at spawn time.";
			property = "KH_ModuleEntitySpawnerAmount";
			defaultValue = "true";
		};
		class KH_ModuleEntitySpawnerMinimumPlayerDistance: Edit
		{
			displayName = "Minimum Player Distance";
			tooltip = "The distance between any player and a chosen spawn position within which the spawner will refuse to spawn an entity.";
			property = "KH_ModuleEntitySpawnerMinimumPlayerDistance";
			defaultValue = "'0'";
		};
		class ModuleDescription: ModuleDescription {};
	};
	class ModuleDescription: ModuleDescription
	{
		description[] = {"Spawns entities in a radius around the position of the module, with the entities being spawned facing the direction of the module. The entities will continue to spawn for as long as the module exists. Activates only once, with a trigger, on the server."};
		sync[] = {};
		position = 1;
		direction = 1;
		optional = 0;
		duplicate = 1;
	};
};