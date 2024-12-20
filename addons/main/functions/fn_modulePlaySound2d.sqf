isNil {
	params [["_logic", objNull], ["_units", []], ["_activated", true]];

	if _activated then {
		[
			[
				[
					_logic getVariable ["KH_ModulePlaySound2DSound", ""], 
					_logic getVariable ["KH_ModulePlaySound2DAttenuationType", 0], 
					parseNumber (_logic getVariable ["KH_ModulePlaySound2DStartTime", "0"])
				]
			],
			{
				params ["_sound"];
				playSound _sound;
			},
			["TARGETS", _units],
			"THIS_FRAME"
		] call KH_fnc_execute;
	};
};

true;