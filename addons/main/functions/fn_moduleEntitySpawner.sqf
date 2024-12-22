isNil {
	params [["_logic", objNull], ["_units", []], ["_activated", false]];

	if _activated then {
		private _transforms = [];

		{
			_transforms pushBack [_x, _x];
		} forEach _units;

		private _radius = parseSimpleArray (_logic getVariable ["KH_ModuleEntitySpawnerRadius", "[]"]);
		private _transformCount = (count _transforms) - 1;
		private _radiuses = [];

		for "_i" from 0 to _transformCount do {
			_radiuses pushBack _radius;
		};

		[
			parseSimpleArray (_logic getVariable ["KH_ModuleEntitySpawnerEntityTypes", "[]"]),
			_transforms,
			_radiuses,
			parseNumber (_logic getVariable ["KH_ModuleEntitySpawnerAmount", "0"]),
			parseNumber (_logic getVariable ["KH_ModuleEntitySpawnerMaximum", "0"]),
			compile (_logic getVariable ["KH_ModuleEntitySpawnerCondition", ""]),
			compile (_logic getVariable ["KH_ModuleEntitySpawnerInitialization", ""]),
			parseSimpleArray (_logic getVariable ["KH_ModuleEntitySpawnerType", "[]"]),
			parseNumber (_logic getVariable ["KH_ModuleEntitySpawnerInterval", "0"])
		] call KH_fnc_entitySpawner;
	};
};

true;