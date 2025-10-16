params [["_object", objNull, [objNull]], ["_video", "", [""]], ["_selection", 0, [0]], ["_audio", "", ["", []]]];
private _skipVariable = generateUid;
_object setObjectTexture [_selection, _video];
_object say3D _audio;
[_video, [10, 10], [1, 1, 1, 1], _skipVariable] call BIS_fnc_playVideo;
[missionNamespace, _skipVariable, clientOwner, true];