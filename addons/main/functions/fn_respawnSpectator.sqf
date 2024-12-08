params [["_state", true], ["_ace", true], ["_freeCamera", false], ["_playerCamera", true]];
KH_var_respawnSpectatorAce = _ace;
publicVariable "KH_var_respawnSpectatorAce";

if (!_freeCamera && !_playerCamera) then {
	_playerCamera = true;
};

KH_var_unconsciousSpectatorFreeCamera = _freeCamera;
publicVariable "KH_var_unconsciousSpectatorFreeCamera";
KH_var_unconsciousSpectatorPlayerCamera = _playerCamera;
publicVariable "KH_var_unconsciousSpectatorPlayerCamera";

if _state then {
	KH_var_respawnSpectatorState = true;

	if (isNil "KH_var_respawnSpectatorSet") then {
		KH_var_respawnSpectatorSet = true;

		[
			"KH_eve_playerRespawned", 
			{
				params ["_unit"];

				if KH_var_respawnSpectatorState then {
					[
						[],
						{
							player setVariable ["KH_var_usingExternalCamera", true, true];

							if KH_var_respawnSpectatorAce then {
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
							}
							else {
								["Initialize", [player], [], true, KH_var_unconsciousSpectatorFreeCamera, KH_var_unconsciousSpectatorPlayerCamera] call BIS_fnc_EGSpectator;

								[
									{
										!(isNull (findDisplay 49));
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
								}, 
								[]
							] call CBA_fnc_execNextFrame;

							systemChat "Press the <ESC> key to exit spectator mode.";
						},
						_unit,
						"THIS_FRAME"
					] call KH_fnc_execute;
				};
			}
		] call CBA_fnc_addEventHandler;
	};
}
else {
	KH_var_respawnSpectatorState = false;
};

true;