isNil {
	params [["_logic", objNull], ["_units", []], ["_activated", false]];

	if _activated then {
		[
			_units, 
			_logic,
			_logic,
			_logic getVariable ["KH_ModuleTeleportEject", false],
			parseNumber (_logic getVariable ["KH_ModuleTeleportTransition", "0"]),
			_logic getVariable ["KH_ModuleTeleportHeal", false],
			parseNumber (_logic getVariable ["KH_ModuleTeleportFreefallHeight", "0"]),
			compile (_logic getVariable ["KH_ModuleTeleportInit", ""])
		] call KH_fnc_teleport;
	};
};

true;