params ["_entity", "_screenMultiplier", "_proximity", "_distance", "_conditionServer", "_conditionPlayer", "_trueFunction", ["_falseFunction", {}], ["_repeatable", false], ["_interval", 0.5], ["_shared", true]];
private _id = format ["KH_var_%1", [0, 36, "ALPHANUMERIC"] call KH_fnc_generateSymbols];
private _firstTrigger = [missionNamespace, "KH_var_visualTriggerFirst", false, false] call KH_fnc_atomicVariable;
private _conditionVariable = [missionNamespace, "KH_var_visualTriggerCondition", false, false] call KH_fnc_atomicVariable;
private _playerVariable = [missionNamespace, "KH_var_visualTriggerPlayerPrevious", false, false] call KH_fnc_atomicVariable;
private _entityVariable = [missionNamespace, "KH_var_visualTriggerEntityPrevious", false, false] call KH_fnc_atomicVariable;
missionNamespace setVariable [_id, "ACTIVE", true];
	
[
	[_entity, _distance, _conditionPlayer, _interval, _id], 
	{
		params ["_entity", "_distance", "_conditionPlayer", "_interval", "_id"];
		
		[
			{
				private _id = _args select 3;
				private _idState = missionNamespace getVariable [_id, "ACTIVE"];

				if (_idState != "INACTIVE") then {
					switch true do {
						case (_idState == "ACTIVE"): {
							_args params ["_entity", "_distance", "_conditionPlayer"];

							if ([] call _conditionPlayer) then {						
								if !(isNil "_entity") then {
									if (((player distance _entity) < _distance) && (alive player) && !(player getVariable ["KH_var_usingExternalCamera", false]) && !(player in ([["CURATORS"], true] call KH_fnc_getClients))) then {
										["KH_eve_visualTrigger", [worldToScreen (ASLToAGL (getPosASL _entity)), player]] call CBA_fnc_serverEvent;
									};
								}
								else {
									[_handle] call CBA_fnc_removePerFrameHandler;
								};
							};
						};

						case (_idState == "TERMINATE"): {
							[_handle] call CBA_fnc_removePerFrameHandler;
						};		
					};
				};
			},
			_interval, 
			[_entity, _distance, _conditionPlayer, _id]
		] call CBA_fnc_addPerFrameHandler;
	},
	["JIP", "PLAYERS", _entity, true, false],
	"THIS_FRAME"
] call KH_fnc_execute;

[
	"KH_eve_visualTrigger", 
	{
		private _id = _thisArgs select 12;
		private _idState = missionNamespace getVariable [_id, "ACTIVE"];
		
		if (_idState != "INACTIVE") then {
			switch true do {
				case (_idState == "ACTIVE"): {
					params ["_screenPos", "_currentPlayer"];
					_thisArgs params ["_entity", "_screenMultiplier", "_proximity", "_conditionServer", "_trueFunction", "_falseFunction", "_repeatable", "_shared", "_firstTrigger", "_conditionVariable", "_playerVariable", "_entityVariable"];
					
					if ([] call _conditionServer) then {
						if !(isNil "_entity") then {
							private _conditionVariableId = format ["%1_%2", _conditionVariable, getPlayerUID _currentPlayer];
												
							if ((_currentPlayer distance _entity) < _proximity) then {
								_entity setVariable [_conditionVariableId, true];
							}
							else {
								if (_screenPos isEqualTo []) then {
									_entity setVariable [_conditionVariableId, false];
								}
								else {
									private _screenPosX = _screenPos select 0;
									private _screenPosY = _screenPos select 1;

									if ((_screenPosX > (0.5 - _screenMultiplier)) && (_screenPosX < (0.5 + _screenMultiplier)) && (_screenPosY > (0.5 - _screenMultiplier)) && (_screenPosY < (0.5 + _screenMultiplier))) then {
										private _entityOffsetAsl = (getPosASL _entity) vectorAdd [0, 0, 0.1];
										private _playerEyes = eyePos _currentPlayer;
										
										if ((([_entity, "VIEW", objNull] checkVisibility [_playerEyes, eyePos _entity]) > 0) || (([_entity, "VIEW", objNull] checkVisibility [_playerEyes, _entityOffsetAsl]) > 0) || (!(terrainIntersectASL [_playerEyes, _entityOffsetAsl]) && !(lineIntersects [_playerEyes, _entityOffsetAsl, _currentPlayer, _entity]))) then {
											_entity setVariable [_conditionVariableId, true];
										}
										else {
											_entity setVariable [_conditionVariableId, false];
										};
									}
									else {
										_entity setVariable [_conditionVariableId, false];
									};
								};
							};
							
							private _condition = false;
							
							{
								if ((_x find (toLower _conditionVariable)) != -1) then {
									if (_entity getVariable [_x, false]) then {
										_condition = true;
										break;
									};
								};
							} forEach (allVariables _entity);
							
							if _shared then {
								if _condition then {
									if !(_entity getVariable [_entityVariable, false]) then {
										_entity setVariable [_firstTrigger, true];
										[_currentPlayer, _entity, ["PUBLIC_HANDLER", _id]] call _trueFunction;
									};
								}
								else {
									if (_entity getVariable [_entityVariable, false]) then {
										if (_entity getVariable [_firstTrigger, false]) then {
											[_currentPlayer, _entity, ["PUBLIC_HANDLER", _id]] call _falseFunction;
										};
										
										if !_repeatable then {
											missionNamespace setVariable [_id, "TERMINATE", true];
										};
									};
								};
							}
							else {
								if _condition then {
									if !(_currentPlayer getVariable [_playerVariable, false]) then {
										_entity setVariable [_firstTrigger, true];
										[_currentPlayer, _entity, ["PUBLIC_HANDLER", _id]] call _trueFunction;
									};
								}
								else {
									if (_currentPlayer getVariable [_playerVariable, false]) then {
										if (_entity getVariable [_firstTrigger, false]) then {
											[_currentPlayer, _entity, ["PUBLIC_HANDLER", _id]] call _falseFunction;
										};
										
										if !_repeatable then {
											missionNamespace setVariable [_id, "TERMINATE", true];
										};	
									};	
								};
							};
							
							_entity setVariable [_entityVariable, _condition];
							_currentPlayer setVariable [_playerVariable, _condition];
						}
						else {
							[_thisType, _thisId] call CBA_fnc_removeEventHandler;
						};
					};
				};

				case (_idState == "TERMINATE"): {
					[_thisType, _thisId] call CBA_fnc_removeEventHandler;
				};		
			};
		};
	},
	[_entity, _screenMultiplier, _proximity, _conditionServer, _trueFunction, _falseFunction, _repeatable, _shared, _firstTrigger, _conditionVariable, _playerVariable, _entityVariable, _id]
] call CBA_fnc_addEventHandlerArgs;

["PUBLIC_HANDLER", _id];