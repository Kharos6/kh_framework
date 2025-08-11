params ["_state", "_uniforms", "_vests", "_headgear", "_vehicles", ["_setCaptive", false]];

if (isNil "KH_var_disguiseSideUniforms") then {
	KH_var_disguiseSideUniforms = [[], [], []];
};

if (isNil "KH_var_disguiseSideVests") then {
	KH_var_disguiseSideVests = [[], [], []];
};

if (isNil "KH_var_disguiseSideHeadgear") then {
	KH_var_disguiseSideHeadgear = [[], [], []];
};

private _i = 0;

for "_i" from 0 to 2 do {
	private _uniformsArray = (KH_var_disguiseSideUniforms select _i);
	_uniformsArray insert [-1, _uniforms select _i, true];
	KH_var_disguiseSideUniforms set [_i, _uniformsArray];
	private _vestsArray = (KH_var_disguiseSideVests select _i);
	_vestsArray insert [-1, _headgear select _i, true];
	KH_var_disguiseSideVests set [_i, _vestsArray];
	private _headgearArray = (KH_var_disguiseSideHeadgear select _i);
	_headgearArray insert [-1, _headgear select _i, true];
	KH_var_disguiseSideHeadgear set [_i, _headgearArray];
};

KH_var_disguiseSetCaptive = _setCaptive;

if _state then {
	KH_var_dynamicDisguiseState = true;
	
	if (isNil "KH_var_dynamicDisguiseSet") then {
		KH_var_dynamicDisguiseSet = true;

		[
			{
				if KH_var_dynamicDisguiseState then {
					{
						if (isNil {_x getVariable 'KH_var_disguiseOriginalSide'}) then {
							_x setVariable ["KH_var_disguiseOriginalSide", side (group _x)];
						};

						private _player = _x;
						private _originalSide = _x getVariable ["KH_var_disguiseOriginalSide", side (group _player)];
						private _uniformIndex = -1;
						private _vestIndex = -1;
						private _headgearIndex = -1;
						private _currentSide = sideUnknown;

						{
							if ((uniform _player) in _x) then {
								_uniformIndex = KH_var_disguiseSideUniforms find _x;
								break;
							}
						} forEach KH_var_disguiseSideUniforms;

						if (((vest _player) == "") || (KH_var_disguiseSideVests isEqualTo [[], [], []])) then {
							_vestIndex = _uniformIndex;
						}
						else {
							{
								if ((vest _player) in _x) then {
									_vestIndex = KH_var_disguiseSideVests find _x;
									break;
								}
							} forEach KH_var_disguiseSideVests;
						};

						if (((headgear _player) == "") || (KH_var_disguiseSideHeadgear isEqualTo [[], [], []])) then {
							_headgearIndex = _uniformIndex;
						}
						else {
							{
								if ((headgear _player) in _x) then {
									_headgearIndex = KH_var_disguiseSideHeadgear find _x;
									break;
								}
							} forEach KH_var_disguiseSideHeadgear;
						};
						
						if ((_uniformIndex == _headgearIndex) && (_uniformIndex == _vestIndex)) then {
							switch _uniformIndex do {
								case 0: {
									_currentSide = west;
								};

								case 1: {
									_currentSide = east;
								};
							
								case 2: {
									_currentSide = resistance;
								};
							};
						};

						{
							if !(_x getVariable ["KH_var_disguiseChecker", false]) then {
								_x setVariable ["KH_var_disguiseChecker", true];
								
								_x addEventHandler [
									"Dammaged", 
									{
										private _unit = _this select 0;
										private _instigator = _this select 5;
										
										if ((_instigator getVariable ["KH_var_disguiseState", false]) && (((side (group _instigator)) == (side (group _unit))) || (((side _instigator)) == sideFriendly))) then {
											[
												{
													params ["_instigator"];	
													private _instigatorVisible = false;
													
													{
														if (!(isPlayer _x) && (alive _x) && ((side (group _x)) == (side (group _instigator))) && ((([_instigator, "VIEW", objectParent _instigator] checkVisibility [eyePos _instigator, eyePos _x]) > 0) || (([_instigator, "VIEW", objectParent _instigator] checkVisibility [(getPosASL _instigator) vectorAdd [0, 0, 1], eyePos _x]) > 0))) then {
															_instigatorVisible = true;
															break;
														};
													} forEach (_instigator nearEntities ["Man", 1000]);
													
													if _instigatorVisible then {
														_instigator setVariable ["KH_var_disguiseState", false];
														_instigator setVariable ["KH_var_disguiseDetected", true];
													};									
												}, 
												[_instigator], 
												3
											] call CBA_fnc_waitAndExecute;
										};
									}
								];
							};
						} forEach ([["REGULAR"], false, false] call KH_fnc_getUnits);

						if !(_player getVariable ["KH_var_disguiseDetected", false]) then {
							if (_currentSide != sideUnknown) then {
								_player setVariable ["KH_var_disguiseState", true];
							};
						}
						else {
							if !(_player getVariable ["KH_var_disguiseRecoveryChecker", false]) then {
								_player setVariable ["KH_var_disguiseRecoveryChecker", true];
								
								[
									{
										_args params ["_player", "_currentSide"];
										private _instigatorVisible = false;
										
										if !(_player getVariable ["KH_var_disguiseDetected", false]) then {
											_player setVariable ["KH_var_disguiseRecoveryChecker", false];
											[_handle] call CBA_fnc_removePerFrameHandler;
										}
										else {
											{
												if ((!(isPlayer _x) && (alive _x) && ((side (group _x)) == _currentSide)) && ((([_player, "VIEW", objectParent _player] checkVisibility [eyePos _player, eyePos _x]) > 0) || (([_player, "VIEW", objectParent _player] checkVisibility [(getPosASL _player) vectorAdd [0, 0, 1], eyePos _x]) > 0))) then {
													_instigatorVisible = true;
													break;
												};
											} forEach (_player nearEntities ["Man", 1000]);
											
											if !_instigatorVisible then {
												_player setVariable ["KH_var_disguiseDetected", false];
												_player setVariable ["KH_var_disguiseRecoveryChecker", false];
												[_handle] call CBA_fnc_removePerFrameHandler;
											};
										};
									},
									60, 
									[_player, _currentSide]
								] call CBA_fnc_addPerFrameHandler;
							};
						};
						
						if !(_player getVariable ["KH_var_disguiseState", false]) then {
							if ((side (group _player)) != _originalSide) then {
								[_player] joinSilent (createGroup [_originalSide, true]);

								[
									[],
									{
										systemChat "Someone saw me. I am no longer disguised.";
									},
									_player,
									true
								] call KH_fnc_execute;
								
								if KH_var_disguiseSetCaptive then {
									[
										[],
										{
											player setCaptive false;
										},
										_player,
										true
									] call KH_fnc_execute;
								};
							};
						}
						else {
							if ((side (group _player)) != _currentSide) then {
								private _playerVisible = false;

								{
									if ((!(isPlayer _x) && (alive _x) && ((side (group _x)) == _currentSide) && ((([_player, "VIEW", objectParent _player] checkVisibility [eyePos _player, eyePos _x]) > 0) || (([_player, "VIEW", objectParent _player] checkVisibility [(getPosASL _player) vectorAdd [0, 0, 1], eyePos _x]) > 0))) || (_player getVariable ["KH_var_disguiseRecoveryChecker", false])) then {
										_playerVisible = true;
										break;
									};
								} forEach (_player nearEntities ["Man", 1000]);

								if !_playerVisible then {
									[_player] joinSilent (createGroup [_currentSide, true]);

									[
										[_currentSide],
										{
											params ["_currentSide"];
											systemChat (format ["No one saw me. I am now disguised as the %1 side.", _currentSide]);
										},
										_player,
										true
									] call KH_fnc_execute;
									
									if KH_var_disguiseSetCaptive then {
										[
											[],
											{
												player setCaptive true;
											},
											_player,
											true
										] call KH_fnc_execute;
									};
								};
							};
						};
					} forEach KH_var_allPlayerUnits;
				};
			},
			1, 
			[]
		] call CBA_fnc_addPerFrameHandler;
	};
}
else {
	KH_var_dynamicDisguiseState = false;
};

true;