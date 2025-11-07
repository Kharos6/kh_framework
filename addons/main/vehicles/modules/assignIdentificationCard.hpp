class KH_ModuleAssignIdentificationCard: Module_F
{
	author = "Kharos";
	scope = 2;
	scopeCurator = 1;
	canSetArea = 0;
	canSetAreaHeight = 0;
	canSetAreaShape = 0;
	displayName = "Assign Identification Card";
	category = "KH_Modules";
	function = "KH_fnc_moduleAssignIdentificationCard";
	functionPriority = 1;
	isGlobal = 0;
	isTriggerActivated = 0;
	isDisposable = 1;
	is3DEN = 0;
	curatorInfoType = "";
	curatorInfoTypeEmpty = "";
	curatorCanAttach = 0;
	icon = "\a3\Modules_F_Curator\Data\iconMissionName_ca.paa";
	class Attributes: AttributesBase
	{
		class KH_ModuleAssignIdentificationCardName: Edit
		{
			displayName = "Name";
			tooltip = "Name.";
			property = "KH_ModuleAssignIdentificationCardName";
			defaultValue = "''";
		};
    	class KH_ModuleAssignIdentificationCardGender: Edit
		{
			displayName = "Gender";
			tooltip = "Gender.";
			property = "KH_ModuleAssignIdentificationCardGender";
			defaultValue = "''";
		};
		class KH_ModuleAssignIdentificationCardRace: Edit
		{
			displayName = "Race";
			tooltip = "Race.";
			property = "KH_ModuleAssignIdentificationCardRace";
			defaultValue = "''";
		};
    	class KH_ModuleAssignIdentificationCardDateOfBirth: Edit
		{
			displayName = "Date Of Birth";
			tooltip = "Date of birth.";
			property = "KH_ModuleAssignIdentificationCardDateOfBirth";
			defaultValue = "''";
		};
		class KH_ModuleAssignIdentificationCardProfession: Edit
		{
			displayName = "Profession";
			tooltip = "Profession.";
			property = "KH_ModuleAssignIdentificationCardProfession";
			defaultValue = "''";
		};
    	class KH_ModuleAssignIdentificationCardEthnos: Edit
		{
			displayName = "Ethnos";
			tooltip = "Ethnos or nationality.";
			property = "KH_ModuleAssignIdentificationCardEthnos";
			defaultValue = "''";
		};
		class KH_ModuleAssignIdentificationCardCardNumber: Edit
		{
			displayName = "Card Number";
			tooltip = "Card number.";
			property = "KH_ModuleAssignIdentificationCardCardNumber";
			defaultValue = "''";
		};
    	class KH_ModuleAssignIdentificationCardDateOfIssue: Edit
		{
			displayName = "Date Of Issue";
			tooltip = "Date of issue.";
			property = "KH_ModuleAssignIdentificationCardDateOfIssue";
			defaultValue = "''";
		};
		class KH_ModuleAssignIdentificationCardDateOfExpiry: Edit
		{
			displayName = "Date Of Expiry";
			tooltip = "Date of expiry.";
			property = "KH_ModuleAssignIdentificationCardDateOfExpiry";
			defaultValue = "''";
		};
		class ModuleDescription: ModuleDescription {};
	};
	class ModuleDescription: ModuleDescription
	{
		description[] = {"Adds an action to the synchronized units that allows players to view their identification card. Any attribute can be left empty to generate a random one. Activates only once, without a trigger, on the server."};
		sync[] = {"AnyBrain"};
		position = 0;
		direction = 0;
		optional = 0;
		duplicate = 1;
	};
};