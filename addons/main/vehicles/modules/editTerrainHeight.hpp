class KH_ModuleEditTerrainHeight: Module_F
{
	author = "Kharos";
	scope = 2;
	scopeCurator = 1;
	canSetArea = 1;
	canSetAreaHeight = 0;
	canSetAreaShape = 1;
	displayName = "Edit Terrain Height";
	category = "KH_Modules";
	function = "KH_fnc_moduleEditTerrainHeight";
	functionPriority = 1;
	isGlobal = 0;
	isTriggerActivated = 1;
	isDisposable = 0;
	is3DEN = 1;
	curatorInfoType = "";
	curatorInfoTypeEmpty = "";
	curatorCanAttach = 0;
	icon = "iconmodule";
    class AttributeValues
    {
        size3[] = {100, 100, -1};
        isRectangle = 0;
    };
	class Attributes: AttributesBase
	{
		class KH_ModuleEditTerrainHeightCurve: EditMulti5
		{
			displayName = "Curve";
			tooltip = "Arrays where the first element is an array that represents the affected area and the second element is a scalar that represents the terrain height modification magnitude. The height array must be comprised of two scalars where both are values from 0 to 1; 0 represents the edge of the area while 1 represents the center of the area. The terrain height modification magnitude scalar represents the magnitude of the terrain height modification within the affected area defined by the height array.";
			property = "KH_ModuleEditTerrainHeightCurve";
			defaultValue = "''";
		};
		class KH_ModuleEditTerrainHeightApplicationType: Combo
		{
			displayName = "Application Type";
			tooltip = "Type of height application.";
			property = "KH_ModuleEditTerrainHeightApplicationType";
			typeName = "STRING";
			defaultValue = "'ADD'";
			class Values
			{
				class KH_Add {name = "ADD"; value = "ADD";};
                class KH_Multiply {name = "MULTIPLY"; value = "MULTIPLY";};
                class KH_Set {name = "SET"; value = "SET";};
			};
		};
		class KH_ModuleEditTerrainHeightCurveType: Combo
		{
			displayName = "Curve Type";
			tooltip = "Type of interpolation used between the different curve points.";
			property = "KH_ModuleEditTerrainHeightCurveType";
			typeName = "STRING";
			defaultValue = "'LINEAR'";
			class Values
			{
                class KH_Linear {name = "LINEAR"; value = "LINEAR";};
                class KH_Bezier {name = "BEZIER"; value = "BEZIER";};
                class KH_None {name = "NONE"; value = "NONE";};
			};
		};
		class KH_ModuleEditTerrainHeightChangeDuration: Edit
		{
			displayName = "Change Duration";
			tooltip = "Time it takes for the terrain height to take effect. Set to 0 for instantaneous changes.";
			property = "KH_ModuleEditTerrainHeightChangeDuration";
			defaultValue = "'0'";
		};
		class KH_ModuleEditTerrainHeightAnimationCurveType: Combo
		{
			displayName = "Animation Curve Type";
			tooltip = "Type of interpolation used for animating the terrain height change.";
			property = "KH_ModuleEditTerrainHeightAnimationCurveType";
			typeName = "STRING";
			defaultValue = "'LINEAR'";
			class Values
			{
                class KH_Linear {name = "LINEAR"; value = "LINEAR";};
                class KH_Bezier {name = "BEZIER"; value = "BEZIER";};
                class KH_None {name = "NONE"; value = "NONE";};
			};
		};
		class KH_ModuleEditTerrainHeightAdjustObjectHeight: Checkbox
		{
			displayName = "Adjust Object Height";
			tooltip = "True adjusts the height of objects to match terrain height changes.";
			property = "KH_ModuleEditTerrainHeightAdjustObjectHeight";
			defaultValue = "true";
		};
		class KH_ModuleEditTerrainHeightShowChanges: Checkbox
		{
			displayName = "Show Changes";
			tooltip = "True shows the terrain changes in the editor.";
			property = "KH_ModuleEditTerrainHeightShowChanges";
			defaultValue = "true";
		};
		class ModuleDescription: ModuleDescription {};
	};
	class ModuleDescription: ModuleDescription
	{
		description[] = {"Edits terrain height within an area. Activates multiple times, with a trigger, on the server."};
		sync[] = {};
		position = 1;
		direction = 1;
		optional = 0;
		duplicate = 1;
	};
};