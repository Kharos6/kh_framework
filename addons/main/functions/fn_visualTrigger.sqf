params [
	["_entity", objNull, [objNull]], 
	["_screenPercentage", 0.5, [0]], 
	["_minimumDistance", 1, [0]], 
	["_maximumDistance", 1000, [0]], 
	["_conditionServer", {true;}, [{}]], 
	["_conditionPlayer", {true;}, [{}]], 
	["_trueFunctionServer", {}, [{}]], 
	["_falseFunctionServer", {}, [{}]],
	["_trueFunctionPlayer", {}, [{}]], 
	["_falseFunctionPlayer", {}, [{}]], 
	["_repeatable", false, [true]], 
	["_interval", 0.5, [0]], 
	["_shared", true, [true]]
];

private _event = generateUid;
private _triggerId = generateUid;
private _firstTrigger = generateUid;
private _playerVariable = generateUid;
private _entityVariable = generateUid;
private _conditionReference = generateUid;
_entity setVariable [_triggerId, true, true];
_entity setVariable [_firstTrigger, false];
_entity setVariable [_playerVariable, false];
_entity setVariable [_entityVariable, false];
_entity setVariable [_conditionReference, createHashMap];
	
private _triggerHandler = [
	[_entity, _maximumDistance, _conditionPlayer, _interval, _event, _triggerId], 
	{
		params ["_entity", "_maximumDistance", "_conditionPlayer", "_interval", "_event", "_triggerId"];
		
		[
			[_entity, _maximumDistance, _conditionPlayer, _event, _triggerId],
			{
				params ["_entity", "_maximumDistance", "_conditionPlayer", "_event", "_triggerId"];

				if !(_entity getVariable _triggerId) exitWith {
					[_handlerId] call KH_fnc_removeHandler;
				};

				if ([_entity] call _conditionPlayer) then {						
					if !(isNull _entity) then {
						if (
							((player distance _entity) < _maximumDistance) && 
							(alive player) && 
							(isNull curatorCamera)
						   ) then {
							[_event, [player]] call CBA_fnc_serverEvent;
						};
					}
					else {
						[_handlerId] call KH_fnc_removeHandler;
					};
				};
			},
			true,
			_interval,
			false 
		] call KH_fnc_execute;
	},
	"PLAYERS",
	true,
	["JIP", _entity, true, ""]
] call KH_fnc_execute;

private _eventHandler = [
	"CBA",
	_event,
	[
		_entity, 
		_screenPercentage, 
		_minimumDistance, 
		_conditionServer, 
		_trueFunctionServer, 
		_falseFunctionServer,
		_trueFunctionPlayer,
		_falseFunctionPlayer,
		_repeatable, 
		_shared, 
		_firstTrigger, 
		_playerVariable, 
		_entityVariable, 
		_triggerHandler,
		_entity getVariable _conditionReference
	],
	{
		params ["_currentPlayer"];

		_args params [
			"_entity", 
			"_screenPercentage", 
			"_minimumDistance", 
			"_conditionServer",
			"_trueFunctionServer", 
			"_falseFunctionServer",
			"_trueFunctionPlayer",
			"_falseFunctionPlayer",
			"_repeatable", 
			"_shared", 
			"_firstTrigger", 
			"_playerVariable", 
			"_entityVariable", 
			"_triggerHandler",
			"_conditionReference"
		];
		
		if ([_entity] call _conditionServer) then {
			if (isNull _entity) exitWith {
				[_handlerId] call KH_fnc_removeHandler;
			};

			private _conditionVariableId = getPlayerUID _currentPlayer;
								
			if ((_currentPlayer distance _entity) < _minimumDistance) then {
				_conditionReference set [_conditionVariableId, true];
			}
			else {
				if !([_currentPlayer, unitAimPositionVisual _entity, _currentPlayer, _screenPercentage, 0, _maximumDistance, true] call KH_fnc_getPositionVisibility) then {
					_conditionReference set [_conditionVariableId, false];
				}
				else {
					_conditionReference set [_conditionVariableId, true];
				};
			};
			
			private _condition = false;
			
			{
				if _y then {
					_condition = true;
					break;
				};
			} forEach _conditionReference;
			
			if _shared then {
				if _condition then {
					if !(_entity getVariable _entityVariable) then {
						_entity setVariable [_firstTrigger, true];
						[_currentPlayer, _entity] call _trueFunctionServer;
						[[_entity], _trueFunctionPlayer, _currentPlayer, true, false] call KH_fnc_execute;
					};
				}
				else {
					if (_entity getVariable _entityVariable) then {
						if (_entity getVariable [_firstTrigger, false]) then {
							[_currentPlayer, _entity] call _falseFunctionServer;
							[[_entity], _falseFunctionPlayer, _currentPlayer, true, false] call KH_fnc_execute;
						};
						
						if !_repeatable then {
							[_triggerHandler] call KH_fnc_removeHandler;
							[_handlerId] call KH_fnc_removeHandler;
						};
					};
				};
			}
			else {
				if _condition then {
					if !(_currentPlayer getVariable _playerVariable) then {
						_entity setVariable [_firstTrigger, true];
						[_currentPlayer, _entity] call _trueFunctionServer;
						[[_entity], _trueFunctionPlayer, _currentPlayer, true, false] call KH_fnc_execute;
					};
				}
				else {
					if (_currentPlayer getVariable _playerVariable) then {
						if (_entity getVariable [_firstTrigger, false]) then {
							[_currentPlayer, _entity] call _falseFunctionServer;
							[[_entity], _falseFunctionPlayer, _currentPlayer, true, false] call KH_fnc_execute;
						};
						
						if !_repeatable then {
							[_triggerHandler] call KH_fnc_removeHandler;
							[_handlerId] call KH_fnc_removeHandler;
						};	
					};	
				};
			};
			
			_entity setVariable [_entityVariable, _condition];
			_currentPlayer setVariable [_playerVariable, _condition];
		};	
	}
] call KH_fnc_addEventHandler;

[_triggerHandler, _eventHandler, [_entity, _triggerId, true]];