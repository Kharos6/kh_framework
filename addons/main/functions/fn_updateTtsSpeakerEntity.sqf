params [
    ["_entity", objNull, [objNull]], 
    ["_text", "", [""]], 
    ["_volume", 1, [0]], 
    ["_speed", 1, [0]], 
    ["_speakerId", 0, [0]], 
    ["_maximumDistance", 100, [0]], 
    ["_functionGenerated", {}, [{}]], 
    ["_functionFinished", {}, [{}]],
    ["_subtitleName", "", [""]],
    ["_override", false, [true]]
];

[
    [_entity, _text, _volume, _speed, _speakerId, _maximumDistance, _functionGenerated, _functionFinished, _subtitleName, _override],
    {
        params ["_entity", "_text", "_volume", "_speed", "_speakerId", "_maximumDistance", "_functionGenerated", "_functionFinished", "_subtitleName", "_override"];
        private _speaker = ["KH_var_ttsSpeaker_", hashValue _entity] joinString "";
        private _argumentsId = ["KH_var_ttsSpeakerArguments_", hashValue _entity] joinString "";
        missionNamespace setVariable [_argumentsId, [_volume, _maximumDistance, _functionGenerated, _functionFinished]];

        private _canUpdate = if (ttsIsPlaying _speaker) then {
            if _override then {
                ttsStopSpeaker _speaker;

                if (_text isNotEqualTo "") then {
                    private _worldDir = (positionCameraToWorld [0, 0, 0]) vectorFromTo (unitAimPositionVisual _entity);
                    private _cameraForward = getCameraViewDirection KH_var_playerUnit;
                    private _cameraRight = vectorNormalized (_cameraForward vectorCrossProduct [0, 0, 1]);
                    private _cameraUp = vectorNormalized (_cameraRight vectorCrossProduct _cameraForward);
                    
                    ttsSpeak [
                        _speaker, 
                        _text, 
                        _worldDir vectorDotProduct _cameraRight, 
                        _worldDir vectorDotProduct _cameraForward, 
                        _worldDir vectorDotProduct _cameraUp, 
                        _volume * (linearConversion [0, _maximumDistance, ((positionCameraToWorld [0, 0, 0]) vectorDistance (unitAimPositionVisual _entity)), 1, 0, true]), 
                        _speed,
                        _speakerId
                    ];

                    true;
                }
                else {
                    false;
                };
            }
            else {
                false;
            };
        }
        else {
            if (_text isNotEqualTo "") then {
                private _worldDir = (positionCameraToWorld [0, 0, 0]) vectorFromTo (unitAimPositionVisual _entity);
                private _cameraForward = getCameraViewDirection KH_var_playerUnit;
                private _cameraRight = vectorNormalized (_cameraForward vectorCrossProduct [0, 0, 1]);
                private _cameraUp = vectorNormalized (_cameraRight vectorCrossProduct _cameraForward);
                
                ttsSpeak [
                    _speaker, 
                    _text, 
                    _worldDir vectorDotProduct _cameraRight, 
                    _worldDir vectorDotProduct _cameraForward, 
                    _worldDir vectorDotProduct _cameraUp, 
                    _volume * (linearConversion [0, _maximumDistance, ((positionCameraToWorld [0, 0, 0]) vectorDistance (unitAimPositionVisual _entity)), 1, 0, true]), 
                    _speed,
                    _speakerId
                ];

                true;
            }
            else {
                false;
            };
        };

        if _canUpdate then {            
            private _generationHandler = [
                "CBA",
                "KH_eve_ttsGenerated",
                [_entity, _speaker, _argumentsId, _maximumDistance, _subtitleName],
                {
                    params ["_speakerId", "_text"];
                    _args params ["_entity", "_speaker", "_argumentsId", "_maximumDistance", "_subtitleName"];

                    if (_speakerId isEqualTo _speaker) then {
                        missionNamespace setVariable [
                            _speaker,
                            [
                                [_entity, _speaker, _argumentsId],
                                {
                                    params ["_entity", "_speaker", "_argumentsId"];
                                    (missionNamespace getVariable _argumentsId) params ["_volume", "_maximumDistance"];

                                    if (((positionCameraToWorld [0, 0, 0]) vectorDistance (unitAimPositionVisual _entity)) < _maximumDistance) then {
                                        private _worldDir = (positionCameraToWorld [0, 0, 0]) vectorFromTo (unitAimPositionVisual _entity);
                                        private _cameraForward = getCameraViewDirection KH_var_playerUnit;
                                        private _cameraRight = vectorNormalized (_cameraForward vectorCrossProduct [0, 0, 1]);
                                        private _cameraUp = vectorNormalized (_cameraRight vectorCrossProduct _cameraForward);

                                        ttsUpdateSpeaker [
                                            _speaker, 
                                            _worldDir vectorDotProduct _cameraRight, 
                                            _worldDir vectorDotProduct _cameraForward, 
                                            _worldDir vectorDotProduct _cameraUp, 
                                            _volume * (linearConversion [0, _maximumDistance, ((positionCameraToWorld [0, 0, 0]) vectorDistance (unitAimPositionVisual _entity)), 1, 0, true])
                                        ];
                                    };
                                },
                                true,
                                0,
                                false
                            ] call KH_fnc_execute
                        ];

                        if ((_subtitleName isNotEqualTo "") && (((positionCameraToWorld [0, 0, 0]) vectorDistance (unitAimPositionVisual _entity)) < _maximumDistance)) then {
                            [_subtitleName, _text, 0] call KH_fnc_displaySubtitle;
                        };

                        call ((missionNamespace getVariable _argumentsId) select 2);                        
                        [_handlerId] call KH_fnc_removeHandler;
                    };
                }
            ] call KH_fnc_addEventHandler;

            [
                "CBA",
                "KH_eve_ttsFinished",
                [_speaker, _generationHandler, _argumentsId, _subtitleName],
                {
                    params ["_speakerId", "_stopped"];
                    _args params ["_speaker", "_generationHandler", "_argumentsId", "_subtitleName"];

                    if (_speakerId isEqualTo _speaker) then {
                        if _stopped then {
                            [_generationHandler] call KH_fnc_removeHandler;
                            call ((missionNamespace getVariable _argumentsId) select 3);
                            [_handlerId] call KH_fnc_removeHandler;
                        }
                        else {
                            call ((missionNamespace getVariable _argumentsId) select 3);
                            [_handlerId] call KH_fnc_removeHandler;
                        };

                        if !(missionNamespace isNil _speaker) then {
                            [missionNamespace getVariable _speaker] call KH_fnc_removeHandler;
                        };

                        if (_subtitleName isNotEqualTo "") then {
                            [_subtitleName, "", 0] call KH_fnc_displaySubtitle;
                        };
                    };
                }
            ] call KH_fnc_addEventHandler;
        };
    },
    "PLAYERS",
    true,
    false
] call KH_fnc_execute;