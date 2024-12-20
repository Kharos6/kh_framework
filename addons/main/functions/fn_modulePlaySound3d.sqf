isNil {
	params [["_logic", objNull], ["_units", []], ["_activated", true]];

	if _activated then {
		[
			[
				_logic,
				[
					_logic getVariable ["KH_ModulePlaySound3DSound", ""], 
					parseNumber (_logic getVariable ["KH_ModulePlaySound3DDistance", "0"]),
					parseNumber (_logic getVariable ["KH_ModulePlaySound3DPitch", "1"]),
					_logic getVariable ["KH_ModulePlaySound3DAttenuationType", 0],
					0,
					_logic getVariable ["KH_ModulePlaySound3DSimulateSpeed", false]
				]
			],
			{
				params ["_logic", "_sound"];
				_logic say3D _sound;
			},
			["TARGETS", _units],
			"THIS_FRAME"
		] call KH_fnc_execute;
	};
};

true;