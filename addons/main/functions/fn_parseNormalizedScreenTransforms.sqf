params [["_transforms", [0, 0, 100, 100], [[]]], ["_square", false, [true]]];
_transforms params [["_positionX", 0, [0]], ["_positionY", 0, [0]], ["_sizeX", 100, [0]], ["_sizeY", 100, [0]]];

[
    (safeZoneX + safeZoneW * (_positionX * 0.01)),
    (safeZoneY + safeZoneH * (_positionY * 0.01)),
    (((safeZoneW / safeZoneH) * safeZoneH) * (_sizeX * 0.01)) + 0.00015,
    if _square then {
        (((safeZoneH / safeZoneW) * safeZoneW) * (_sizeY * 0.01)) + 0.00015;
    }
    else {
        ((((safeZoneH / safeZoneW) * safeZoneW) * (safeZoneW / safeZoneH) * safeZoneW / safeZoneH) * (_sizeY * 0.01)) + 0.00015;
    }
];