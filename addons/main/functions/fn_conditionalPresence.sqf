params [["_object", objNull, [objNull]], ["_event", "", ["", objNull]], ["_init", {}, [{}]], ["_invert", false, [true]]];

if (_event isEqualType objNull) then {
	execute [
		[_object, _event, _init, _invert, triggerActivated _event],
		{
			params ["_object", "_event", "_init", "_invert", "_previousActivation"];

			if ((isNull _object) || (isNull _event)) exitWith {
				[_handlerId] call KH_fnc_removeHandler;
			};

			private _triggerActivated = triggerActivated _event;

			if (_previousActivation isNotEqualTo _triggerActivated) then {
				_this set [4, _triggerActivated];
				private _present = [_triggerActivated, !_triggerActivated] select _invert;
				_object enableSimulationGlobal _present;
				_object hideObjectGlobal !_present;

				execute [
					[_object, _triggerActivated, _init],
					{
						params ["_object", "_state", "_init"];
						[_object, _state] call _init;
					},
					_object,
					true,
					false
				];
			};
		},
		true,
		triggerInterval _event,
		false
	];
}
else {
	[
		"CBA",
		_event,
		[_object, _init, _invert],
		{
			params ["_state"];
			_args params ["_object", "_init", "_invert"];

			if (isNull _object) exitWith {
				[_handlerId] call KH_fnc_removeHandler;
			};

			private _present = [_state, !_state] select _invert;
			_object enableSimulationGlobal _present;
			_object hideObjectGlobal !_present;

			execute [
				[_object, _state, _init],
				{
					params ["_object", "_state", "_init"];
					[_object, _state] call _init;
				},
				_object,
				true,
				false
			];
		}
	] call KH_fnc_addEventHandler;
};

nil;