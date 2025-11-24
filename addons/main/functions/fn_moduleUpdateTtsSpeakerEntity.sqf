isNil {
    params [["_logic", objNull, [objNull]], ["_units", [], [[]]], ["_activated", true, [true]]];

    if _activated then {
        [
            [
                _units,
                _logic getVariable ["KH_ModuleUpdateTTSSpeakerEntityText", ""],
                parseNumber (_logic getVariable ["KH_ModuleUpdateTTSSpeakerEntityVolume", "1"]),
                parseNumber (_logic getVariable ["KH_ModuleUpdateTTSSpeakerEntitySpeed", "1"]),
                parseNumber (_logic getVariable ["KH_ModuleUpdateTTSSpeakerEntitySpeakerID", "0"]),
                parseNumber (_logic getVariable ["KH_ModuleUpdateTTSSpeakerEntityMaximumDistance", "100"]),
                compile (_logic getVariable ["KH_ModuleUpdateTTSSpeakerEntityGeneratedFunction", ""]),
                compile (_logic getVariable ["KH_ModuleUpdateTTSSpeakerEntityFinishedFunction", ""]),
                _logic getVariable ["KH_ModuleUpdateTTSSpeakerEntitySubtitleName", ""],
                _logic getVariable ["KH_ModuleUpdateTTSSpeakerEntityOverride", false]
            ],
            {
                params ["_units", "_text", "_volume", "_speed", "_speakerId", "_maximumDistance", "_functionGenerated", "_functionFinished", "_subtitleName", "_override"];

                {
                    [_x, _text, _volume, _speed, _speakerId, _maximumDistance, _functionGenerated, _functionFinished, _subtitleName, _override] call KH_fnc_updateTtsSpeakerEntity;
                } forEach _units;
            },
            "PLAYERS",
            true,
            false
        ] call KH_fnc_execute;

        if (_logic getVariable ["KH_ModuleUpdateTTSSpeakerEntityActivateOnce", true]) then {
            deleteVehicle _logic;
        };
    };
};

nil;