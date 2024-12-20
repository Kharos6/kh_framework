isNil {
	params [["_logic", objNull], ["_units", []], ["_activated", true]];

	if _activated then {
		[
			[
				[
					_logic getVariable ["KH_ModulePlayMusicMusic", ""], 
					parseNumber (_logic getVariable ["KH_ModulePlayMusicStartTime", "0"])
				]
			],
			{
				params ["_music"];
				playMusic _music;
			},
			["TARGETS", _units],
			"THIS_FRAME"
		] call KH_fnc_execute;
	};
};

true;