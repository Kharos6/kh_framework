params [["_safeZoneCoordinates", [0, 0, 0, 0], [[]]], ["_square", false, [true]]];
_safeZoneCoordinates params [["_safeZoneX", 0, [0]], ["_safeZoneY", 0, [0]], ["_safeZoneW", 0, [0]], ["_safeZoneH", 0, [0]]];

[
    ((_safeZoneX - safeZoneX) / safeZoneW) * 100,
    ((_safeZoneY - safeZoneY) / safeZoneH) * 100,
    ((_safeZoneW - 0.00015) / safeZoneW) * 100,
    if _square then {
        (((safeZoneW / safeZoneH) * _safeZoneW) - 0.00015) - (safeZoneY * 100);
    }
    else {
        ((_safeZoneH - 0.00015) / ((safeZoneW * safeZoneW) / safeZoneH)) * 100;
    }
];