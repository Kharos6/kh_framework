class KH_ModuleCameraSequence: Module_F
{
	scope = 2;
	scopeCurator = 1;
	canSetArea = 0;
	canSetAreaHeight = 0;
	canSetAreaShape = 0;
	displayName = "Camera Sequence";
	category = "KH_Modules";
	function = "KH_fnc_moduleCameraSequence";
	functionPriority = 1;
	isGlobal = 0;
	isTriggerActivated = 1;
	isDisposable = 0;
	is3DEN = 0;
	icon = "iconcamera";
	curatorInfoType = "";
	curatorInfoTypeEmpty = "";
	curatorCanAttach = 0;
	class Attributes: AttributesBase
	{
		class KH_ModuleCameraSequencePositions: Edit
		{
			displayName = "Positions";
			tooltip = "Array of arrays that dictate the position of the camera, in format: <[_position (STRING or ARRAY), _attach (BOOLEAN)]>. <_position> is either a string of the variable name of the entity which the position of will be used for the camera, or an <AGL> position array in format: <[x, y, z]>. <_attach> is a boolean where <true> instantly attaches the camera to <_position>, and <false> transitions the camera to <_position>, only effective and necessary if <_position> is an entity.";
			property = "KH_ModuleCameraSequencePositions";
			defaultValue = "'[]'";
		};
		class KH_ModuleCameraSequenceTargets: Edit
		{
			displayName = "Targets";
			tooltip = "Array of either strings of variable names of entities that the camera will track, or <AGL> position arrays in format <[x, y, z]>.";
			property = "KH_ModuleCameraSequenceTargets";
			defaultValue = "'[]'";
		};
		class KH_ModuleCameraSequenceFOVs: Edit
		{
			displayName = "FOVs";
			tooltip = "Array of scalars of FOV values of the camera, from <0.01> to <8.5>. Default FOV is <0.75>.";
			property = "KH_ModuleCameraSequenceFOVs";
			defaultValue = "'[]'";
		};
		class KH_ModuleCameraSequenceCommitTimes: Edit
		{
			displayName = "Commit Times";
			tooltip = "Array of scalars dictating the transition duration from one shot to the next, in seconds.";
			property = "KH_ModuleCameraSequenceCommitTimes";
			defaultValue = "'[]'";
		};
		class KH_ModuleCameraSequenceDurations: Edit
		{
			displayName = "Durations";
			tooltip = "Array of scalars dictating the duration of each shot, in seconds.";
			property = "KH_ModuleCameraSequenceDurations";
			defaultValue = "'[]'";
		};
		class KH_ModuleCameraSequenceVisionTypes: Edit
		{
			displayName = "Vision Types";
			tooltip = "Array of Scalars that dictate the imaging type of the camera, can be one of the following: <-2> = Night vision. <-1> = Normal vision. <0> = White hot. <1> = Black hot. <2> = Light green hot, darker green cold. <3> = Black hot, darker green cold. <4> = Light red hot, darker red cold. <5> = Black hot, darker red cold. <6> = White Hot, darker red cold. <7> = Thermal shades of red and green, bodies are white.";
			property = "KH_ModuleCameraSequenceVisionTypes";
			defaultValue = "'[]'";
		};
		class KH_ModuleCameraSequenceCinematicBorders: Checkbox
		{
			displayName = "Cinematic Borders";
			tooltip = "<true> adds black bars on the top and bottom of the screen for the duration of the sequence.";
			property = "KH_ModuleCameraSequenceCinematicBorders";
			defaultValue = "false";
		};
		class KH_ModuleCameraSequenceDisableUserInput: Checkbox
		{
			displayName = "Disable User Input";
			tooltip = "<true> disables user input until the camera sequence is concluded.";
			property = "KH_ModuleCameraSequenceDisableUserInput";
			defaultValue = "false";
		};
		class ModuleDescription: ModuleDescription {};
	};
	class ModuleDescription: ModuleDescription
	{
		description[] = {"Plays a camera sequence for the synchronized players. The attributes of each camera are interpolated based on index value equivalents from the different arrays of attributes. For example, the third element in <Positions> will take effect on the same shot as the third element in <Targets>. Must be activated through a trigger or triggers. Can activate multiple times. Activated on the server."};
		sync[] = {"AnyPlayer"};
		position = 0;
		direction = 0;
		optional = 0;
		duplicate = 1;
	};
};