isNil {
	params [["_logic", objNull], ["_units", []], ["_activated", true]];

	if _activated then {
		[
			[
				_logic getVariable ["KH_ModuleDisplaySubtitleSpeaker", ""],
				_logic getVariable ["KH_ModuleDisplaySubtitleText", ""],
				parseNumber (_logic getVariable ["KH_ModuleDisplaySubtitleDuration", "0"])
			],
			"KH_fnc_displaySubtitle",
			["TARGETS", _units],
			"THIS_FRAME"
		] call KH_fnc_execute;
	};
};

true;