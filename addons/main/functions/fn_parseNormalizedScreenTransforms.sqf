params [["_transforms", [0, 0, 100, 100], [[]]], ["_square", false, [true]]];
_transforms params [["_positionX", 0, [0]], ["_positionY", 0, [0]], ["_sizeX", 100, [0]], ["_sizeY", 100, [0]]];
private _aspectCorrection = 1.77778 / (getResolution select 4);

[
    (safeZoneX + safeZoneW * (_positionX * 0.01)),
    (safeZoneY + safeZoneH * (_positionY * 0.01)),
    ((((safeZoneW / safeZoneH) * safeZoneH) * (_sizeX * 0.01)) + 0.00015),
    if _square then {
        (safeZoneW * (_sizeX * 0.01) * _aspectCorrection) + 0.00015;
    } 
    else {
        (safeZoneW * (_sizeX * 0.01)) + 0.00015;
    },
    if _square then {
        ((safeZoneW * (_sizeY * 0.01) * _aspectCorrection + 0.00015) * pixelH / pixelW);
    } 
    else {
        (safeZoneH * (_sizeY * 0.01)) + 0.00015;
    }
];