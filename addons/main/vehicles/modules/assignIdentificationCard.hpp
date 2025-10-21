class KH_ModuleAssignIdentificationCard: Module_F
{
	scope = 2;
	scopeCurator = 1;
	canSetArea = 0;
	canSetAreaHeight = 0;
	canSetAreaShape = 0;
	displayName = "Assign Identification Card";
	category = "KH_Modules";
	function = "KH_fnc_moduleAssignIdentificationCard";
	functionPriority = 1;
	isGlobal = 2;
	isTriggerActivated = 0;
	isDisposable = 1;
	is3DEN = 0;
	icon = "\a3\modules_f_curator\data\iconendmission_ca.paa";
	class Attributes: AttributesBase
	{
		class Name: Edit
		{
			displayName = "Name";
			tooltip = "Name.";
			property = "KH_ModuleAssignIdentificationCardName";
			defaultValue = "''";
		};
    	class Gender: Edit
		{
			displayName = "Gender";
			tooltip = "Gender.";
			property = "KH_ModuleAssignIdentificationCardGender";
			defaultValue = "''";
		};
		class Race: Edit
		{
			displayName = "Race";
			tooltip = "Race.";
			property = "KH_ModuleAssignIdentificationCardRace";
			defaultValue = "''";
		};
    	class DateOfBirth: Edit
		{
			displayName = "Date Of Birth";
			tooltip = "Date of birth.";
			property = "KH_ModuleAssignIdentificationCardDateOfBirth";
			defaultValue = "''";
		};
		class Profession: Edit
		{
			displayName = "Profession";
			tooltip = "Profession.";
			property = "KH_ModuleAssignIdentificationCardProfession";
			defaultValue = "''";
		};
    	class Ethnos: Edit
		{
			displayName = "Ethnos";
			tooltip = "Ethnos or nationality.";
			property = "KH_ModuleAssignIdentificationCardEthnos";
			defaultValue = "''";
		};
		class CardNumber: Edit
		{
			displayName = "Card Number";
			tooltip = "Card number.";
			property = "KH_ModuleAssignIdentificationCardCardNumber";
			defaultValue = "''";
		};
    	class DateOfIssue: Edit
		{
			displayName = "Date Of Issue";
			tooltip = "Date of issue.";
			property = "KH_ModuleAssignIdentificationCardDateOfIssue";
			defaultValue = "''";
		};
		class DateOfExpiry: Edit
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
		description[] = {"Adds an action to the synchronized units that allows players to view their identification card. Activates only once, without a trigger, globally and persistently."};
		sync[] = {"AnyPerson"};
		position = 0;
		direction = 0;
		optional = 0;
		duplicate = 0;
	};
};