params [["_state", true, [true]], ["_uniforms", [], [[]]], ["_vests", [], [[]]], ["_headgear", [], [[]]], ["_vehicles", [], [[]]], ["_setCaptive", false, [true]]];
KH_var_dynamicDisguiseState = _state;

if (isNil "KH_var_disguiseSideUniforms") then {
	KH_var_disguiseSideUniforms = [[], [], []];
};

if (isNil "KH_var_disguiseSideVests") then {
	KH_var_disguiseSideVests = [[], [], []];
};

if (isNil "KH_var_disguiseSideHeadgear") then {
	KH_var_disguiseSideHeadgear = [[], [], []];
};

for "_i" from 0 to 2 do {
	(KH_var_disguiseSideUniforms select _i) insert [-1, _uniforms select _i, true];
	(KH_var_disguiseSideVests select _i) insert [-1, _vests select _i, true];
	(KH_var_disguiseSideHeadgear select _i) insert [-1, _headgear select _i, true];
};

KH_var_disguiseSetCaptive = _setCaptive;

if _state then {	
	if (isNil "KH_var_dynamicDisguiseSet") then {
		KH_var_dynamicDisguiseSet = true;

		[
			[],
			{
				if !KH_var_dynamicDisguiseState exitWith {};

				{
					private _player = _x;

					if (isNil {_player getVariable "KH_var_disguiseOriginalSide";}) then {
						_player setVariable ["KH_var_disguiseOriginalSide", side (group _player)];
					};

					private _originalSide = _player getVariable "KH_var_disguiseOriginalSide";
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

					if (((vest _player) isEqualTo "") || (KH_var_disguiseSideVests isEqualTo [[], [], []])) then {
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

					if (((headgear _player) isEqualTo "") || (KH_var_disguiseSideHeadgear isEqualTo [[], [], []])) then {
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
					
					if ((_uniformIndex isEqualTo _headgearIndex) && (_uniformIndex isEqualTo _vestIndex)) then {
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
									private _unit = param [0];
									private _instigator = param [5];
									
									if (
										(_instigator getVariable ["KH_var_disguiseState", false]) && 
										(((side (group _instigator)) isEqualTo (side (group _unit))) || (((side _instigator)) isEqualTo sideFriendly))
									   ) then {
										[
											[_instigator],
											{
												params ["_instigator"];	
												private _instigatorVisible = false;
												
												{
													if (
														!(isPlayer _x) && 
														(alive _x) && 
														((side (group _x)) isEqualTo (side (group _instigator))) && 
														(
														 (([_instigator, "VIEW", objectParent _instigator] checkVisibility [eyePos _instigator, eyePos _x]) > 0) || 
														 (([_instigator, "VIEW", objectParent _instigator] checkVisibility [AGLToASL (unitAimPosition _player), eyePos _x]) > 0)
														)
													   ) then {
														_instigatorVisible = true;
														break;
													};
												} forEach (_instigator nearEntities ["Man", 1000]);
												
												if _instigatorVisible then {
													_instigator setVariable ["KH_var_disguiseState", false];
													_instigator setVariable ["KH_var_disguiseDetected", true];
												};									
											},
											true,
											"3",
											false
										] call KH_fnc_execute;
									};
								}
							];
						};
					} forEach allUnits;

					if !(_player getVariable ["KH_var_disguiseDetected", false]) then {
						if (_currentSide isNotEqualTo sideUnknown) then {
							_player setVariable ["KH_var_disguiseState", true];
						};
					}
					else {
						if !(_player getVariable ["KH_var_disguiseRecoveryChecker", false]) then {
							_player setVariable ["KH_var_disguiseRecoveryChecker", true];
							
							[
								[_player, _currentSide],
								{
									params ["_player", "_currentSide"];
									private _instigatorVisible = false;
									
									if !(_player getVariable ["KH_var_disguiseDetected", false]) then {
										_player setVariable ["KH_var_disguiseRecoveryChecker", false];
										[_handlerId] call KH_fnc_removeHandler;
									}
									else {
										{
											if (
												(!(isPlayer _x) && (alive _x) && ((side (group _x)) isEqualTo _currentSide)) && 
												(
												 (([_player, "VIEW", objectParent _player] checkVisibility [eyePos _player, eyePos _x]) > 0) || 
												 (([_player, "VIEW", objectParent _player] checkVisibility [AGLToASL (unitAimPosition _player), eyePos _x]) > 0)
												)
											   ) then {
												_instigatorVisible = true;
												break;
											};
										} forEach (_player nearEntities ["Man", 1000]);
										
										if !_instigatorVisible then {
											_player setVariable ["KH_var_disguiseDetected", false];
											_player setVariable ["KH_var_disguiseRecoveryChecker", false];
											[_handlerId] call KH_fnc_removeHandler;
										};
									};
								},
								true,
								60,
								false
							] call KH_fnc_execute;
						};
					};
					
					if !(_player getVariable ["KH_var_disguiseState", false]) then {
						if ((side (group _player)) isNotEqualTo _originalSide) then {
							[_player] joinSilent (createGroup [_originalSide, true]);

							[
								[],
								{
									systemChat "Someone saw me. I am no longer disguised.";
								},
								_player,
								true,
								false
							] call KH_fnc_execute;
							
							if KH_var_disguiseSetCaptive then {
								[
									[],
									{
										player setCaptive false;
									},
									_player,
									true,
									false
								] call KH_fnc_execute;
							};
						};
					}
					else {
						if ((side (group _player)) isNotEqualTo _currentSide) then {
							private _playerVisible = false;

							{
								if (
									(
									 !(isPlayer _x) && 
									 (alive _x) && 
									 ((side (group _x)) isEqualTo _currentSide) && 
									 (
									  (([_player, "VIEW", objectParent _player] checkVisibility [eyePos _player, eyePos _x]) > 0) || 
									  (([_player, "VIEW", objectParent _player] checkVisibility [(getPosASL _player) vectorAdd [0, 0, 1], eyePos _x]) > 0)
									 )
									) || 
									(_player getVariable ["KH_var_disguiseRecoveryChecker", false])
								   ) then {
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
										systemChat (["No one saw me. I am now disguised as the ", _currentSide, " side."] joinString "");
									},
									_player,
									true,
									false
								] call KH_fnc_execute;
								
								if KH_var_disguiseSetCaptive then {
									[
										[],
										{
											player setCaptive true;
										},
										_player,
										true,
										false
									] call KH_fnc_execute;
								};
							};
						};
					};
				} forEach KH_var_allPlayerUnits;
			},
			true,
			1,
			false
		] call KH_fnc_execute;
	};
};

nil;