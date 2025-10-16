params [["_object", objNull, [objNull]], ["_position", [0, 0, 0], [[], objNull]], ["_target", [0, 0, 0], [[], objNull]], ["_selection", 0, [0, ""]], ["_renderTarget", 1, [0]]];
private _camera = "camera" camCreate [0, 0, 0];

if (_position isEqualType []) then {
	_camera camSetPos _position;
}
else {
	_camera attachTo [_position, [0, 0, 0]];
};

_camera camSetTarget _target;
_renderTarget = (_renderTarget min 8) max 1;

switch _renderTarget do {
	case 1: {
		_object setObjectTexture [_selection, "#(argb,512,512,1)r2t(khrt1,1)"];
		_camera cameraEffect ["Internal", "BACK", "khrt1"];		
	};
	
	case 2: {
		_object setObjectTexture [_selection, "#(argb,512,512,1)r2t(khrt2,1)"];
		_camera cameraEffect ["Internal", "BACK", "khrt2"];		
	};
	
	case 3: {
		_object setObjectTexture [_selection, "#(argb,512,512,1)r2t(khrt3,1)"];
		_camera cameraEffect ["Internal", "BACK", "khrt3"];		
	};
	
	case 4: {
		_object setObjectTexture [_selection, "#(argb,512,512,1)r2t(khrt4,1)"];
		_camera cameraEffect ["Internal", "BACK", "khrt4"];		
	};
	
	case 5: {
		_object setObjectTexture [_selection, "#(argb,512,512,1)r2t(khrt5,1)"];
		_camera cameraEffect ["Internal", "BACK", "khrt5"];		
	};
	
	case 6: {
		_object setObjectTexture [_selection, "#(argb,512,512,1)r2t(khrt6,1)"];
		_camera cameraEffect ["Internal", "BACK", "khrt6"];		
	};
	
	case 7: {
		_object setObjectTexture [_selection, "#(argb,512,512,1)r2t(khrt7,1)"];
		_camera cameraEffect ["Internal", "BACK", "khrt7"];		
	};
	
	case 8: {
		_object setObjectTexture [_selection, "#(argb,512,512,1)r2t(khrt8,1)"];
		_camera cameraEffect ["Internal", "BACK", "khrt8"];		
	};
};