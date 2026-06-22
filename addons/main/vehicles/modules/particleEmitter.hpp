class KH_ModuleParticleEmitter: Module_F
{
	author = "Kharos";
	scope = 2;
	scopeCurator = 1;
	canSetArea = 0;
	canSetAreaHeight = 0;
	canSetAreaShape = 0;
	displayName = "Particle Emitter";
	category = "KH_Modules";
	function = "KH_fnc_moduleParticleEmitter";
	functionPriority = 1;
	isGlobal = 0;
	isTriggerActivated = 1;
	isDisposable = 0;
	is3DEN = 1;
	curatorInfoType = "";
	curatorInfoTypeEmpty = "";
	curatorCanAttach = 0;
	icon = "iconmodule";
	class Attributes: AttributesBase
	{
		class KH_ModuleParticleEmitterParticle: Default
		{
			control = "KH_ParticleEmitterDropdown";
			displayName = "";
			tooltip = "";
			property = "KH_ModuleParticleEmitterParticle";
			defaultValue = "['', 0]";
		};
		class KH_ModuleParticleEmitterDuration: Edit
		{
			displayName = "Duration";
			tooltip = "Duration of the particle emission. Set to 0 for infinite duration.";
			property = "KH_ModuleParticleEmitterDuration";
			defaultValue = "'0'";
		};
		class KH_ModuleParticleEmitterCondition: EditCodeMulti5
		{
			displayName = "Condition";
			tooltip = "Unscheduled code, executed locally to each client, that must return true in order for the particle to exist. Passed arguments available through _this are: [_logic (OBJECT)]";
			property = "KH_ModuleParticleEmitterCondition";
			defaultValue = "'true'";
		};
		class ModuleDescription: ModuleDescription {};
	};
	class ModuleDescription: ModuleDescription
	{
		description[] = {"Creates a particle effect. Activates multiple times, with a trigger, on the server."};
		sync[] = {};
		position = 1;
		direction = 0;
		optional = 1;
		duplicate = 1;
	};
};