params [["_object", objNull, [objNull]], ["_event", "", [""]], ["_init", {}, [{}]], ["_invert", false, [true]]];

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