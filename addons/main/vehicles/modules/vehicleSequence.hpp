class KH_ModuleVehicleSequence: Module_F
{
	scope = 2;
	scopeCurator = 1;
	canSetArea = 0;
	canSetAreaHeight = 0;
	canSetAreaShape = 0;
	displayName = "Vehicle Sequence";
	category = "KH_Modules";
	function = "KH_fnc_moduleVehicleSequence";
	functionPriority = 1;
	isGlobal = 0;
	isTriggerActivated = 1;
	isDisposable = 0;
	is3DEN = 0;
	icon = "\a3\modules_f_curator\data\iconcas_ca.paa";
	curatorInfoType = "";
	curatorInfoTypeEmpty = "";
	curatorCanAttach = 0;
	class Attributes: AttributesBase
	{
		class KH_ModuleVehicleSequenceMovementData: Edit
		{
			displayName = "Movement Data";
			tooltip = "Path to the file containing movement data, including its extension.";
			property = "KH_ModuleVehicleSequenceMovementData";
			defaultValue = "''";
		};
		class KH_ModuleVehicleSequenceFiringData: Edit
		{
			displayName = "Firing Data";
			tooltip = "Path to the file containing firing data, including its extension.";
			property = "KH_ModuleVehicleSequenceFiringData";
			defaultValue = "''";
		};
		class KH_ModuleVehicleSequenceDisableDamage: Checkbox
		{
			displayName = "Disable Damage";
			tooltip = "<true> prevents the vehicle from receiving damage while the sequence is active.";
			property = "KH_ModuleVehicleSequenceDisableDamage";
			defaultValue = "false";
		};
		class KH_ModuleVehicleSequenceEndPosition: Edit
		{
			displayName = "Disable Damage";
			tooltip = "Variable name of the entity to act as the waypoint that this vehicle will move to after the sequence is finished. Can be empty for no end waypoint.";
			property = "KH_ModuleVehicleSequenceEndPosition";
			defaultValue = "''";
		};
		class ModuleDescription: ModuleDescription {};
	};
	class ModuleDescription: ModuleDescription
	{
		description[] = {"Plays a vehicle sequence on the synchronized vehicle. Must be activated through a trigger or triggers. Can activate multiple times. Activated on the server."};
		sync[] = {"AnyVehicle"};
		position = 0;
		direction = 0;
		optional = 0;
		duplicate = 0;
	};
};