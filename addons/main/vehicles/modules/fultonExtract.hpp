class KH_ModuleFultonExtract: Module_F
{
	author = "Kharos";
	scope = 2;
	scopeCurator = 1;
	canSetArea = 0;
	canSetAreaHeight = 0;
	canSetAreaShape = 0;
	displayName = "Fulton Extract";
	category = "KH_Modules";
	function = "KH_fnc_moduleFultonExtract";
	functionPriority = 1;
	isGlobal = 0;
	isTriggerActivated = 0;
	isDisposable = 1;
	is3DEN = 0;
	icon = "a3\Modules_F_Tacops\Data\CivilianPresenceSafeSpot\icon32_ca.paa";
	class Attributes: AttributesBase
	{
		class Name: Edit
		{
			displayName = "Name";
			tooltip = "Name assigned to the object for actions.";
			property = "KH_ModuleFultonExtractName";
			defaultValue = "''";
		};
		class Vehicles: Edit
		{
			displayName = "Vehicles";
			tooltip = "Array of strings of variable names of vehicles that are allowed to activate the fulton baloon.";
			property = "KH_ModuleFultonExtractVehicles";
			defaultValue = "''";
		};
		class Height: Edit
		{
			displayName = "Height";
			tooltip = "Height, in metres, that the fulton baloon will reach as it waits for a designated vehicle. Maximum value is clamped to 100.";
			property = "KH_ModuleFultonExtractHeight";
			defaultValue = "'100'";
		};
		class Distance: Edit
		{
			displayName = "Distance";
			tooltip = "The minimum activation distance, in metres, between a designated vehicle and the fulton baloon, within which the attached units will start being pulled to the vehicle. Minimum value is clamped to 10, though could be more generous depending on the vehicles.";
			property = "KH_ModuleFultonExtractDistance";
			defaultValue = "'5'";
		};
		class MaximumParticipants: Edit
		{
			displayName = "Maximum Participants";
			tooltip = "The maximum amount of people that are allowed to carry a harness for this fulton.";
			property = "KH_ModuleFultonExtractMaximumParticipants";
			defaultValue = "'10'";
		};
		class Duration: Edit
		{
			displayName = "Duration";
			tooltip = "The time it will take for participants to get pulled into the vehicle.";
			property = "KH_ModuleFultonExtractDuration";
			defaultValue = "'15'";
		};
		class ModuleDescription: ModuleDescription {};
	};
	class ModuleDescription: ModuleDescription
	{
		description[] = {"Adds an action to the synchronized objects that allows players to attach a fulton harness to themselves or others, which will then pull them towards the designated vehicle after said vehicle passes within the designated distance of the baloon. Activates only once, without a trigger, on the server."};
		sync[] = {"AnyStaticObject"};
		position = 0;
		direction = 0;
		optional = 0;
		duplicate = 0;
	};
};