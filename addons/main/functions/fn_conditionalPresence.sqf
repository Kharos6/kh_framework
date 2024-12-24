params ["_entity", "_variableName", ["_init", {}], ["_invert", false]];

[
	"CBA",
	_variableName,
	[_entity, _init, _invert],
	{
		params ["_state"];
		_args params ["_entity", "_init", "_invert"];

		if !_invert then {
			if _state then {
				_entity enableSimulationGlobal true;
				_entity hideObjectGlobal false;
			}
			else {
				_entity enableSimulationGlobal false;
				_entity hideObjectGlobal true;
			};
		}
		else {
			if _state then {
				_entity enableSimulationGlobal false;
				_entity hideObjectGlobal true;
			}
			else {
				_entity enableSimulationGlobal true;
				_entity hideObjectGlobal false;
			};
		};
		
		[
			[_entity, _state, _init],
			{
				params ["_entity", "_state", "_init"];
				[_entity, _state] call _init;
			},
			_entity,
			"THIS_FRAME"
		] call KH_fnc_execute;
	}
] call KH_fnc_addEventHandler;

true;