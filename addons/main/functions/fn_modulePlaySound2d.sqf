isNil {
	params [["_logic", objNull], ["_units", []], ["_activated", true]];

	if _activated then {
		{
			if (isPlayer _x) then {
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
					_x,
					"THIS_FRAME"
				] call KH_fnc_execute;
			};
		} forEach _units;
	};
};

true;