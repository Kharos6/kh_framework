isNil {
	params [["_display", displayNull], ["_logic", objNull]];

	[
		["CONTROL", _display displayCtrl 1],
		"ButtonClick",
		[_display, _logic],
		{
			params ["_control"];
			_args params ["_display", "_logic"];
			private _fadeType = 1;
			
			if ("true" in (ctrlText (_display displayCtrl 102))) then {
				_fadeType = true;
			}
			else {
				_fadeType = parseNumber (ctrlText (_display displayCtrl 102));
			};

			[
				[ctrlText (_display displayCtrl 100), ctrlChecked (_display displayCtrl 101), _fadeType, parseNumber (ctrlText (_display displayCtrl 103)), parseSimpleArray (ctrlText (_display displayCtrl 104))],
				"KH_fnc_endMission",
				"SERVER",
				"THIS_FRAME"
			] call KH_fnc_execute;

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