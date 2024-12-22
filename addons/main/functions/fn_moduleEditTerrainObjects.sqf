isNil {
	params [["_logic", objNull], ["_units", []], ["_activated", false]];

	if _activated then {
		[
			parseSimpleArray (_logic getVariable ["KH_ModuleEditTerrainObjectsObjects", "[]"]),
			_logic,
			parseNumber (_logic getVariable ["KH_ModuleEditTerrainObjectsRadius", "0"]),
			parseNumber (_logic getVariable ["KH_ModuleEditTerrainObjectsDamage", "0"]),
			parseNumber (_logic getVariable ["KH_ModuleEditTerrainObjectsChance", "0"]),
			_logic getVariable ["KH_ModuleEditTerrainObjectsRandomDamage", false],
			_logic getVariable ["KH_ModuleEditTerrainObjectsEffects", false],
			_logic getVariable ["KH_ModuleEditTerrainObjectsDynamicSimulation", false],
			_logic getVariable ["KH_ModuleEditTerrainObjectsDisableDamage", false],
			_logic getVariable ["KH_ModuleEditTerrainObjectsConvertToSimple", false],
			compile (_logic getVariable ["KH_ModuleEditTerrainObjectsInitialization", ""])
		] call KH_fnc_editTerrainObjects;
	};
};

true;