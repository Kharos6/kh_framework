isNil {
	params [["_logic", objNull], ["_units", []], ["_activated", false]];

	if _activated then {
		{
			if (isPlayer _x) then {
				[
					[
						_logic getVariable ["KH_ModuleDisplaySubtitleSpeaker", ""],
						_logic getVariable ["KH_ModuleDisplaySubtitleText", ""],
						parseNumber (_logic getVariable ["KH_ModuleDisplaySubtitleDuration", "0"])
					],
					"KH_fnc_displaySubtitle",
					_x,
					"THIS_FRAME"
				] call KH_fnc_execute;
			};
		} forEach _units;
	};
};

true;