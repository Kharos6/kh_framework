params [["_input", userInputDisabled, [true]], ["_stop", true, [true]]];
		
if _input then {
	if userInputDisabled then {
		disableUserInput false;
		
		if _stop then {
			if (isNull (objectParent player)) then {
				[player, ["MOVE_SWITCH_GLOBAL", [""], false], false, false] call KH_fnc_setAnimation;
			};
		};
	};
}
else {
	if !userInputDisabled then {
		disableUserInput true;
		
		if _stop then {
			if (isNull (objectParent player)) then {
				[player, ["MOVE_SWITCH_GLOBAL", [""], false], false, false] call KH_fnc_setAnimation;
			};
		};
	};
};