params ["_state", "_uniforms", "_headgear", ["_setCaptive", false]];
KH_var_disguiseSideUniforms = [_uniforms select 0, _uniforms select 1, _uniforms select 2];
KH_var_disguiseSideHeadgear = [_headgear select 0, _headgear select 1, _headgear select 2];
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
						private _headgearIndex = -1;
						private _currentSide = sideUnknown;

						{
							if ((uniform _player) in _x) then {
								_uniformIndex = KH_var_disguiseSideUniforms find _x;
								break;
							}
						} forEach KH_var_disguiseSideUniforms;

						if ((headgear _player) == "") then {
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
						
						if (_uniformIndex == _headgearIndex) then {
							switch true do {
								case (_uniformIndex == 0): {
									_currentSide = west;
								};

								case (_uniformIndex == 1): {
									_currentSide = east;
								};
							
								case (_uniformIndex == 2): {
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
														if (!(isPlayer _x) && (alive _x) && ((side (group _x)) == (side (group _instigator))) && (([_instigator, "VIEW", objNull] checkVisibility [eyePos _instigator, eyePos _x]) > 0)) then {
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
										_args params ["_player"];
										private _instigatorVisible = false;
										
										if !(_player getVariable ["KH_var_disguiseDetected", false]) then {
											_player setVariable ["KH_var_disguiseRecoveryChecker", false];
											[_handle] call CBA_fnc_removePerFrameHandler;
										}
										else {
											{
												if (((side (group _x)) == _currentSide) && (([_player, "VIEW", objNull] checkVisibility [eyePos _player, eyePos _x]) > 0)) then {
													_instigatorVisible = true;
													break;
												};
											} forEach ([["REGULAR"], false, false] call KH_fnc_getUnits);
											
											if !_instigatorVisible then {
												_player setVariable ["KH_var_disguiseDetected", false];
												_player setVariable ["KH_var_disguiseRecoveryChecker", false];
												[_handle] call CBA_fnc_removePerFrameHandler;
											};
										};
									},
									60, 
									[_player]
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
									"THIS_FRAME"
								] call KH_fnc_execute;
								
								if KH_var_disguiseSetCaptive then {
									[
										[],
										{
											player setCaptive false;
										},
										_player,
										"THIS_FRAME"
									] call KH_fnc_execute;
								};
							};
						}
						else {
							if ((side (group _player)) != _currentSide) then {
								private _playerVisible = false;

								{
									if (!(isPlayer _x) && (alive _x) && (side (group _x) == _currentSide) && (([_player, "VIEW", objNull] checkVisibility [eyePos _player, eyePos _x]) > 0)) then {
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
										"THIS_FRAME"
									] call KH_fnc_execute;
									
									if KH_var_disguiseSetCaptive then {
										[
											[],
											{
												player setCaptive true;
											},
											_player,
											"THIS_FRAME"
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