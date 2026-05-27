params [["_safeZoneCoordinates", [0, 0, 0, 0], [[]]], ["_square", false, [true]]];
_safeZoneCoordinates params [["_safeZoneX", 0, [0]], ["_safeZoneY", 0, [0]], ["_safeZoneW", 0, [0]], ["_safeZoneH", 0, [0]]];
private _aspectCorrection = 1.77778 / (getResolution select 4);

[
    ((_safeZoneX - safeZoneX) / safeZoneW) * 100,
    ((_safeZoneY - safeZoneY) / safeZoneH) * 100,
    if _square then {
        ((_safeZoneW - 0.00015) / (safeZoneW * _aspectCorrection)) * 100;
    } 
    else {
        ((_safeZoneW - 0.00015) / safeZoneW) * 100;
    },
    if _square then {
        ((_safeZoneH * pixelW / pixelH - 0.00015) / (safeZoneW * _aspectCorrection)) * 100;
    } 
    else {
        ((_safeZoneH - 0.00015) / safeZoneH) * 100;
    }
]