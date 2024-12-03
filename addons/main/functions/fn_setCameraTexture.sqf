params ["_screen", "_positionEntity", "_targetEntity", "_texture", "_renderTarget"];
private _camera = "camera" camCreate [0, 0, 0];
_camera attachTo [_positionEntity, [0, 0, 0]];
_camera camSetTarget _targetEntity;

switch true do {
	case (_renderTarget == 1): {
		_screen setObjectTexture [_texture, "#(argb,512,512,1)r2t(khrt1,1)"];
		_camera cameraEffect ["Internal", "BACK", "khrt1"];		
	};
	
	case (_renderTarget == 2): {
		_screen setObjectTexture [_texture, "#(argb,512,512,1)r2t(khrt2,1)"];
		_camera cameraEffect ["Internal", "BACK", "khrt2"];		
	};
	
	case (_renderTarget == 3): {
		_screen setObjectTexture [_texture, "#(argb,512,512,1)r2t(khrt3,1)"];
		_camera cameraEffect ["Internal", "BACK", "khrt3"];		
	};
	
	case (_renderTarget == 4): {
		_screen setObjectTexture [_texture, "#(argb,512,512,1)r2t(khrt4,1)"];
		_camera cameraEffect ["Internal", "BACK", "khrt4"];		
	};
	
	case (_renderTarget == 5): {
		_screen setObjectTexture [_texture, "#(argb,512,512,1)r2t(khrt5,1)"];
		_camera cameraEffect ["Internal", "BACK", "khrt5"];		
	};
	
	case (_renderTarget == 6): {
		_screen setObjectTexture [_texture, "#(argb,512,512,1)r2t(khrt6,1)"];
		_camera cameraEffect ["Internal", "BACK", "khrt6"];		
	};
	
	case (_renderTarget == 7): {
		_screen setObjectTexture [_texture, "#(argb,512,512,1)r2t(khrt7,1)"];
		_camera cameraEffect ["Internal", "BACK", "khrt7"];		
	};
	
	case (_renderTarget == 8): {
		_screen setObjectTexture [_texture, "#(argb,512,512,1)r2t(khrt8,1)"];
		_camera cameraEffect ["Internal", "BACK", "khrt8"];		
	};
};

true;