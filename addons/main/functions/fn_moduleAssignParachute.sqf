isNil {
	params [["_logic", objNull], ["_units", []], ["_activated", true]];

	if _activated then {
		[
			_units, 
			_logic getVariable ["KH_ModuleAssignParachuteSteerable", false], 
			_logic getVariable ["KH_ModuleAssignParachuteEquip", false]
		] call KH_fnc_assignParachute;
	};
};

true;