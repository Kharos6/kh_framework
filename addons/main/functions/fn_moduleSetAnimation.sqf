isNil {
	params [["_logic", objNull], ["_units", []], ["_activated", false]];

	if _activated then {
		private _animation = _logic getVariable ["KH_ModuleSetAnimationAnimation", ""];

		if (("[" in _animation) || ("]" in _animation)) then {
			_animation = parseSimpleArray _animation;
		};

		{
			[
				_x,
				_animation,
				_x,
				_x,
				_x,
				_x,
				_x,
				parseNumber (_logic getVariable ["KH_ModuleSetAnimationDuration", "0"]),
				false,
				false,
				false,
				false,
				_logic getVariable ["KH_ModuleSetAnimationInterruptType", ""],
				""
			] call KH_fnc_setAnimation;
		} forEach _units;
	};
};

true;