params ["_entity", "_variableName", "_initialization", "_invert"];

if (isNil "KH_var_conditionalPresenceVariables") then {
	KH_var_conditionalPresenceVariables = [];
};

if !(_variableName in KH_var_conditionalPresenceVariables) then {
	KH_var_conditionalPresenceVariables pushBack _variableName;

	_variableName addPublicVariableEventHandler {
		[_this select 0, [_this select 1]] call CBA_fnc_serverEvent;
	};
};

[
	_variableName,
	{
		params ["_state"];
		_thisArgs params ["_entity", "_initialization", "_invert"];

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
			[_entity, _state, _initialization],
			{
				params ["_entity", "_state", "_initialization"];
				[_entity, _state] call _initialization;
			},
			_entity,
			"THIS_FRAME"
		] call KH_fnc_execute;
	},
	[_entity, _initialization, _invert]
] call CBA_fnc_addEventHandlerArgs;

true;