params [["_object", objNull, [objNull]], ["_event", "", ["", objNull]], ["_init", {}, [{}]], ["_invert", false, [true]]];

if (_event isEqualType objNull) then {
	_event setVariable ["KH_var_conditionalPresenceActivation", triggerActivated _event];
	
	[
		[_object, _event, _init, _invert],
		{
			params ["_object", "_event", "_init", "_invert"];
			private _triggerActivated = triggerActivated _event;

			if ((_event getVariable ["KH_var_conditionalPresenceActivation", _triggerActivated]) isNotEqualTo _triggerActivated) then {
				_event setVariable ["KH_var_conditionalPresenceActivation", _triggerActivated];

				if !_invert then {
					if _triggerActivated then {
						_object enableSimulationGlobal true;
						_object hideObjectGlobal false;
					}
					else {
						_object enableSimulationGlobal false;
						_object hideObjectGlobal true;
					};
				}
				else {
					if _triggerActivated then {
						_object enableSimulationGlobal false;
						_object hideObjectGlobal true;
					}
					else {
						_object enableSimulationGlobal true;
						_object hideObjectGlobal false;
					};
				};
				
				[
					[_object, triggerActivated _event, _init],
					{
						params ["_object", "_state", "_init"];
						[_object, _state] call _init;
					},
					_object,
					true,
					false
				] call KH_fnc_execute;
			};
		},
		true,
		triggerInterval _event,
		false
	] call KH_fnc_execute;
}
else {
	[
		"CBA",
		_event,
		[_object, _init, _invert],
		{
			params ["_state"];
			_args params ["_object", "_init", "_invert"];

			if !_invert then {
				if _state then {
					_object enableSimulationGlobal true;
					_object hideObjectGlobal false;
				}
				else {
					_object enableSimulationGlobal false;
					_object hideObjectGlobal true;
				};
			}
			else {
				if _state then {
					_object enableSimulationGlobal false;
					_object hideObjectGlobal true;
				}
				else {
					_object enableSimulationGlobal true;
					_object hideObjectGlobal false;
				};
			};
			
			[
				[_object, _state, _init],
				{
					params ["_object", "_state", "_init"];
					[_object, _state] call _init;
				},
				_object,
				true,
				false
			] call KH_fnc_execute;
		}
	] call KH_fnc_addEventHandler;
};