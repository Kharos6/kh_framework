params [["_input", userInputDisabled, [true]], ["_stop", true, [true]], ["_type", true, [true]]];
		
if _input then {
	if _type then {
		if userInputDisabled then {
			disableUserInput false;
			
			if _stop then {
				if (isNull (objectParent player)) then {
					player switchMove [""];
				};
			};
		};
	}
	else {
		if !(isNil "KH_var_toggleUserInputHandler") then {
			[KH_var_toggleUserInputHandler] call KH_fnc_removeHandler;
			KH_var_toggleUserInputHandler = nil;
		};

		closeDialog 2;

		KH_var_toggleUserInputHandler = [
			[],
			{
				if !dialog then {
					createDialog "RscText";
				};
			},
			true,
			0,
			false
		] call KH_fnc_execute;
	};
}
else {
	if _type then {
		if !userInputDisabled then {
			disableUserInput true;
			
			if _stop then {
				if (isNull (objectParent player)) then {
					player switchMove [""];
				};
			};
		};
	}
	else {
		if !(isNil "KH_var_toggleUserInputHandler") then {
			[KH_var_toggleUserInputHandler] call KH_fnc_removeHandler;
			KH_var_toggleUserInputHandler = nil;
		};
		
		closeDialog 2;
	};
};