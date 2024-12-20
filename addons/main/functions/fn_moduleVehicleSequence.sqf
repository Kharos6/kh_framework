isNil {
	params [["_logic", objNull], ["_units", []], ["_activated", true]];

	if _activated then {
		[
			[
				_logic getVariable ["KH_ModuleVehicleSequenceMovementData", ""], 
				_logic getVariable ["KH_ModuleVehicleSequenceFiringData", ""], 
				_logic getVariable ["KH_ModuleVehicleSequenceDisableDamage", false], 
				missionNamespace getVariable [_logic getVariable ["KH_ModuleVehicleSequenceEndPosition", ""], objNull]
			],
			"KH_fnc_vehicleSequence",
			["TARGETS", _units],
			"THIS_FRAME"
		] call KH_fnc_execute;
	};
};

true;