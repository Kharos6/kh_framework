isNil {
	params [["_logic", objNull], ["_units", []], ["_activated", false]];

	if _activated then {
		{
			if (isPlayer _x) then {
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
					_x,
					"THIS_FRAME"
				] call KH_fnc_execute;
			};
		} forEach _units;
	};
};

true;