params [["_state", true], ["_ace", true], ["_freeCamera", false], ["_playerCamera", true], ["_delay", 30]];
KH_var_unconsciousSpectatorAce = _ace;
publicVariable "KH_var_unconsciousSpectatorAce";

if (!_freeCamera && !_playerCamera) then {
	_playerCamera = true;
};

KH_var_unconsciousSpectatorFreeCamera = _freeCamera;
publicVariable "KH_var_unconsciousSpectatorFreeCamera";
KH_var_unconsciousSpectatorPlayerCamera = _playerCamera;
publicVariable "KH_var_unconsciousSpectatorPlayerCamera";
KH_var_unconsciousSpectatorPlayerDelay = _delay;
publicVariable "KH_var_unconsciousSpectatorPlayerDelay";

if _state then {
	KH_var_unconsciousSpectatorState = true;

	if (isNil "KH_var_unconsciousSpectatorSet") then {
		KH_var_unconsciousSpectatorSet = true;

		[
			{				
				if KH_var_unconsciousSpectatorState then {
					{						
						if ((_x getVariable ["ACE_isUnconscious", false]) || ((lifeState _x) == "INCAPACITATED")) then {
							if !(_x getVariable ["KH_var_unconsciousSpectator", false]) then {
								_x setVariable ["KH_var_unconsciousSpectator", true, true];

								[
									[],
									{
										systemChat (format ["You will enter spectator mode if you remain unconscious for %1 seconds.", KH_var_unconsciousSpectatorPlayerDelay]);
									},
									_x,
									"THIS_FRAME"								
								] call KH_fnc_execute;

								[
									{
										params ["_unit"];

										if ((_unit getVariable ["ACE_isUnconscious", false]) || ((lifeState _unit) == "INCAPACITATED")) then {
											[
												[],
												{
													player setVariable ["KH_var_usingExternalCamera", true, true];

													if KH_var_unconsciousSpectatorAce then {
														[true, false, false] call ace_spectator_fnc_setSpectator;
														[[-1, 0], []] call ace_spectator_fnc_updateVisionModes;
														private _addedCameras = [];
														private _removedCameras = [];

														if (!KH_var_unconsciousSpectatorFreeCamera && !KH_var_unconsciousSpectatorPlayerCamera) then {
															_addedCameras append [1, 2];
														}
														else {
															if KH_var_unconsciousSpectatorFreeCamera then {
																_addedCameras pushBack 0;
															}
															else {
																_removedCameras pushBack 0;
															};

															if KH_var_unconsciousSpectatorPlayerCamera then {
																_addedCameras append [1, 2];
															}
															else {
																_removedCameras append [1, 2];
															};
														};

														[_addedCameras, _removedCameras] call ace_spectator_fnc_updateCameraModes;

														[
															{
																(!(player getVariable ["ACE_isUnconscious", false]) && !((lifeState player) == "INCAPACITATED"));
															}, 
															{
																[false, false, false] call ace_spectator_fnc_setSpectator;
															}, 
															[]
														] call CBA_fnc_waitUntilAndExecute;
													}
													else {
														["Initialize", [player], [], true, KH_var_unconsciousSpectatorFreeCamera, KH_var_unconsciousSpectatorPlayerCamera] call BIS_fnc_EGSpectator;

														[
															{
																(!(isNull (findDisplay 49)) || (!(player getVariable ["ACE_isUnconscious", false]) && !((lifeState player) == "INCAPACITATED")));
															}, 
															{
																["Terminate", [player]] call BIS_fnc_EGSpectator;
																(findDisplay 49) closeDisplay 2;
															}, 
															[]
														] call CBA_fnc_waitUntilAndExecute;
													};

													[
														{
															[
																{
																	((isNull curatorCamera) && (isNull (missionNamespace getVariable ["BIS_EGSpectatorCamera_camera", objNull])) && (isNull (missionNamespace getVariable ["BIS_fnc_camera_cam", objNull])) && (isNull (missionNamespace getVariable ["BIS_DEBUG_CAM", objNull])));
																}, 
																{
																	player setVariable ["KH_var_usingExternalCamera", false, true];
																}, 
																[]
															] call CBA_fnc_waitUntilAndExecute;

															[
																{
																	(!(player getVariable ["ACE_isUnconscious", false]) && !((lifeState player) == "INCAPACITATED"));
																}, 
																{
																	player setVariable ["KH_var_unconsciousSpectator", false, true];
																}, 
																[]
															] call CBA_fnc_waitUntilAndExecute;
														}, 
														[]
													] call CBA_fnc_execNextFrame;

													systemChat "Press the <ESC> key to exit spectator mode, or wait until you become conscious.";
												},
												_unit,
												"THIS_FRAME"
											] call KH_fnc_execute;
										}
										else {
											player setVariable ["KH_var_unconsciousSpectator", false, true];
										};
									}, 
									[_x],
									KH_var_unconsciousSpectatorPlayerDelay
								] call CBA_fnc_waitAndExecute;
							};
						}; 
					} forEach KH_var_allPlayerUnits;
				};
			}, 
			0.1, 
			[]
		] call CBA_fnc_addPerFrameHandler;
	};
}
else {
	KH_var_unconsciousSpectatorState = false;
};

true;