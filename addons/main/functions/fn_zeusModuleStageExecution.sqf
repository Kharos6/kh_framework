isNil {
	params [["_display", displayNull], ["_logic", objNull]];

	[
		["CONTROL", _display displayCtrl 1],
		"ButtonClick",
		[_display, _logic],
		{
			params ["_control"];
			_args params ["_display", "_logic"];

			private _handler = [
				["DISPLAY", 46],
				"KeyDown",
				[compile (ctrlText (_display displayCtrl 100)), objectParent _logic],
				{
					params ["_display", "_key"];

					if (_key isEqualTo 0xD2) then {
						_args params ["_function", "_entity"];

						[
							[_function, _entity],
							{
								params ["_function", "_entity"];
								[_entity] call _function;
							},
							"SERVER",
							"THIS_FRAME"
						] call KH_fnc_execute;

						_display displayRemoveEventHandler ["KeyDown", _localId];
					};
				}
			] call KH_fnc_addEventHandler;

			if (isNil "KH_var_stagedExecutions") then {
				KH_var_stagedExecutions = [];
			};

			KH_var_stagedExecutions pushBack _handler;
			deleteVehicle _logic;
			_control ctrlRemoveEventHandler ["ButtonClick", _localId];
		}
	] call KH_fnc_addEventHandler;

	[
		["CONTROL", _display displayCtrl 2],
		"ButtonClick",
		[_logic],
		{
			params ["_control"];
			_args params ["_logic"];
			deleteVehicle _logic;
			_control ctrlRemoveEventHandler ["ButtonClick", _localId];
		}
	] call KH_fnc_addEventHandler;
};

true;