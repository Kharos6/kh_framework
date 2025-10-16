params [
	["_entity", objNull, [objNull]], 
	["_screenMultiplier", 0, [0]], 
	["_proximity", 0, [0]], 
	["_distance", 0, [0]], 
	["_conditionServer", {}, [{}]], 
	["_conditionPlayer", {}, [{}]], 
	["_trueFunction", {}, [{}]], 
	["_falseFunction", {}, [{}]], 
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
	[_entity, _distance, _conditionPlayer, _interval, _event, _triggerId], 
	{
		params ["_entity", "_distance", "_conditionPlayer", "_interval", "_event", "_triggerId"];
		
		[
			[_entity, _distance, _conditionPlayer, _event, _triggerId],
			{
				params ["_entity", "_distance", "_conditionPlayer", "_event", "_triggerId"];

				if !(_entity getVariable _triggerId) exitWith {
					[_handlerId] call KH_fnc_removeHandler;
				};

				if ([_entity] call _conditionPlayer) then {						
					if !(isNull _entity) then {
						if (
							((player distance _entity) < _distance) && 
							(alive player) && 
							!(clientOwner in KH_var_allCuratorMachines)
						   ) then {
							[_event, [worldToScreen (ASLToAGL (getPosASL _entity)), player]] call CBA_fnc_serverEvent;
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
		_screenMultiplier, 
		_proximity, 
		_conditionServer, 
		_trueFunction, 
		_falseFunction,
		_repeatable, 
		_shared, 
		_firstTrigger, 
		_playerVariable, 
		_entityVariable, 
		_triggerHandler,
		_entity getVariable _conditionReference
	],
	{
		params ["_screenPos", "_currentPlayer"];

		_args params [
			"_entity", 
			"_screenMultiplier", 
			"_proximity", 
			"_conditionServer",
			"_trueFunction", 
			"_falseFunction", 
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
								
			if ((_currentPlayer distance _entity) < _proximity) then {
				_conditionReference set [_conditionVariableId, true];
			}
			else {
				if (_screenPos isEqualTo []) then {
					_conditionReference set [_conditionVariableId, false];
				}
				else {
					_screenPos params ["_screenPosX", "_screenPosY"];

					if (
						(_screenPosX > (0.5 - _screenMultiplier)) && 
						(_screenPosX < (0.5 + _screenMultiplier)) && 
						(_screenPosY > (0.5 - _screenMultiplier)) && 
						(_screenPosY < (0.5 + _screenMultiplier))
					   ) then {
						private _playerEyes = eyePos _currentPlayer;
						private _entityPosition = getPosASL _entity;
						
						if (
							(([_entity, "VIEW", objNull] checkVisibility [_playerEyes, _entityPosition]) > 0) ||
							(!(terrainIntersectASL [_playerEyes, _entityPosition]) && !(lineIntersects [_playerEyes, _entityPosition, _currentPlayer, _entity]))
						   ) then {
							_conditionReference set [_conditionVariableId, true];
						}
						else {
							_conditionReference set [_conditionVariableId, false];
						};
					}
					else {
						_conditionReference set [_conditionVariableId, false];
					};
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
						[_currentPlayer, _entity] call _trueFunction;
					};
				}
				else {
					if (_entity getVariable _entityVariable) then {
						if (_entity getVariable [_firstTrigger, false]) then {
							[_currentPlayer, _entity] call _falseFunction;
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
						[_currentPlayer, _entity] call _trueFunction;
					};
				}
				else {
					if (_currentPlayer getVariable _playerVariable) then {
						if (_entity getVariable [_firstTrigger, false]) then {
							[_currentPlayer, _entity] call _falseFunction;
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