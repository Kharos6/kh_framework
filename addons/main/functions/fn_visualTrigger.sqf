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
_entity setVariable [_entityVariable, false];
_entity setVariable [_conditionReference, createHashMap];
	
private _triggerHandler = execute [
	[_entity, _maximumDistance, _conditionPlayer, _interval, _event, _triggerId], 
	{
		params ["_entity", "_maximumDistance", "_conditionPlayer", "_interval", "_event", "_triggerId"];
		
		execute [
			[_entity, _maximumDistance, _conditionPlayer, _event, _triggerId, false],
			{
				params ["_entity", "_maximumDistance", "_conditionPlayer", "_event", "_triggerId", "_previouslyActive"];

				if !(_entity getVariable [_triggerId, false]) exitWith {
					[_handlerId] call KH_fnc_removeHandler;
				};

				private _active = ([_entity] call _conditionPlayer) && ((player distance _entity) <= _maximumDistance) && (alive player) && (isNull curatorCamera);

				if (_active || _previouslyActive) then {
					triggerCbaEvent [_event, [player, _active], "SERVER", false];
				};

				_this set [5, _active];
			},
			true,
			_interval,
			false 
		];
	},
	"PLAYERS",
	true,
	["JIP", _entity, true, ""]
];

private _eventHandler = [
	"CBA",
	_event,
	[
		_entity, 
		_screenPercentage, 
		_minimumDistance, 
		_maximumDistance,
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
		params ["_currentPlayer", "_active"];

		_args params [
			"_entity", 
			"_screenPercentage", 
			"_minimumDistance", 
			"_maximumDistance",
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
		
		if ([_currentPlayer, _entity] call _conditionServer) then {
			if (isNull _entity) exitWith {
				[_handlerId] call KH_fnc_removeHandler;
			};
								
			if !_active then {
				_conditionReference set [getPlayerUID _currentPlayer, false];
			}
			else {
				if ((_currentPlayer distance _entity) < _minimumDistance) then {
					_conditionReference set [getPlayerUID _currentPlayer, true];
				}
				else {
					_conditionReference set [getPlayerUID _currentPlayer, [_currentPlayer, _entity, _currentPlayer, _screenPercentage, 0, _maximumDistance, true] call KH_fnc_getPositionVisibility];
				};
			};

			if _shared then {
				private _condition = false;

				{
					if _y then {
						_condition = true;
						break;
					};
				} forEach _conditionReference;

				if _condition then {
					if !(_entity getVariable _entityVariable) then {
						_entity setVariable [_firstTrigger, true];
						[_currentPlayer, _entity] call _trueFunctionServer;
						execute [[_entity], _trueFunctionPlayer, _currentPlayer, true, false];
					};
				}
				else {
					if (_entity getVariable _entityVariable) then {
						if (_entity getVariable [_firstTrigger, false]) then {
							[_currentPlayer, _entity] call _falseFunctionServer;
							execute [[_entity], _falseFunctionPlayer, _currentPlayer, true, false];
						};
						
						if !_repeatable then {
							[_triggerHandler] call KH_fnc_removeHandler;
							[_handlerId] call KH_fnc_removeHandler;
						};
					};
				};

				_entity setVariable [_entityVariable, _condition];
			}
			else {
				private _condition = _conditionReference getOrDefault [getPlayerUID _currentPlayer, false];

				if _condition then {
					if !(_currentPlayer getVariable [_playerVariable, false]) then {
						_entity setVariable [_firstTrigger, true];
						[_currentPlayer, _entity] call _trueFunctionServer;
						execute [[_entity], _trueFunctionPlayer, _currentPlayer, true, false];
					};
				}
				else {
					if (_currentPlayer getVariable [_playerVariable, false]) then {
						if (_entity getVariable [_firstTrigger, false]) then {
							[_currentPlayer, _entity] call _falseFunctionServer;
							execute [[_entity], _falseFunctionPlayer, _currentPlayer, true, false];
						};
						
						if !_repeatable then {
							[_triggerHandler] call KH_fnc_removeHandler;
							[_handlerId] call KH_fnc_removeHandler;
						};	
					};	
				};

				_currentPlayer setVariable [_playerVariable, _condition];
			};
		};	
	}
] call KH_fnc_addEventHandler;

[_triggerHandler, _eventHandler, [_entity, _triggerId, true]];