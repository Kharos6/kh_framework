params [["_unit", player], ["_fov", 0.75], ["_visionType", -1]];

if (isNil "KH_var_currentCameraPerspectiveCamera") then {
	KH_var_currentCameraPerspectiveCamera = objNull;
};

if !(isNull KH_var_currentCameraPerspectiveCamera) then {
	KH_var_currentCameraPerspectiveCamera cameraEffect ["Terminate", "BACK"];
	camDestroy KH_var_currentCameraPerspectiveCamera;
};

if (_unit != player) then {
	_cameraObject = createVehicle ["KH_HelperSquare", _unit, [], 0, "CAN_COLLIDE"];
	_cameraObject attachTo [_unit, [-0.12, 0, 0.15], "Head", true];	
	_cameraTarget = createVehicle ["KH_HelperSquare", _unit, [], 0, "CAN_COLLIDE"];
	_cameraTarget attachTo [_unit, [0, 15, 0], "Head", true];
	private _camera = "camera" camCreate [0, 0, 0];
	_camera cameraEffect ["Internal", "BACK"];
	_camera attachTo [_cameraObject, [0, 0, 0]];
	_camera camSetTarget _cameraTarget;
	_camera camSetFov _fov;

	switch true do {
		case (_visionType == -2): {
			camUseNVG true;
		};

		case (_visionType == -1): {
			false setCamUseTI 0;
			camUseNVG false;
		};

		case (_visionType >= 0): {
			true setCamUseTI _visionType;
		};
	};

	_camera camCommit 0;
	KH_var_currentCameraPerspectiveCamera = _camera;
	_camera;
};