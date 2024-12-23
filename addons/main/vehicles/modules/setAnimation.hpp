class KH_ModuleSetAnimation: Module_F
{
	scope = 2;
	scopeCurator = 1;
	canSetArea = 0;
	canSetAreaHeight = 0;
	canSetAreaShape = 0;
	displayName = "Set Animation";
	category = "KH_Modules";
	function = "KH_fnc_moduleSetAnimation";
	functionPriority = 1;
	isGlobal = 0;
	isTriggerActivated = 1;
	isDisposable = 0;
	is3DEN = 0;
	icon = "\a3\modules_f_tacops\data\civilianpresenceunit\icon32_ca.paa";
	curatorInfoType = "";
	curatorInfoTypeEmpty = "";
	curatorCanAttach = 0;
	class Attributes: AttributesBase
	{
		class KH_ModuleSetAnimationAnimation: Edit
		{
			displayName = "Animation";
			tooltip = "Either the name of the animation to play on the unit, or an array in format of the <switchMove> command syntax.";
			property = "KH_ModuleSetAnimationAnimation";
			defaultValue = "''";
		};
		class KH_ModuleSetAnimationDuration: Edit
		{
			displayName = "Duration";
			tooltip = "Length of the animation, in seconds. <0> is an infinite loop. <-1> freezes the first frame of the chosen animation, then unsimulates and kills the unit. Anything greater than <0> is how long the animation will last before exiting.";
			property = "KH_ModuleSetAnimationDuration";
			defaultValue = "''";
		};
		class KH_ModuleSetAnimationInterruptType: Combo
		{
			displayName = "Interrupt Type";
			tooltip = "<'NONE'> prevents the animation from being interrupted until it is finished. <'UNIT'> interrupts the animation only if the unit is hit. <'GROUP'> interrupts the animation when the unit feels threatened, and the unit's group must be in <Safe> mode in order to maintain the animation.";
			property = "KH_ModuleSetAnimationInterruptType";
			typeName = "STRING";
			defaultValue = "'NONE'";
			class Values
			{
				class KH_None {name = "NONE"; value = "NONE";};
				class KH_Unit {name = "UNIT"; value = "UNIT";};
				class KH_Group {name = "GROUP"; value = "GROUP";};
			};
		};
		class ModuleDescription: ModuleDescription {};
	};
	class ModuleDescription: ModuleDescription
	{
		description[] = {"Plays an animation on the synchronized units. Can be exclusively activated through a trigger or triggers. Can activate multiple times. Activated on the server."};
		sync[] = {"AnyPerson"};
		position = 0;
		direction = 0;
		optional = 0;
		duplicate = 1;
	};
};