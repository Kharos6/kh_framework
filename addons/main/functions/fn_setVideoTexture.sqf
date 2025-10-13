params [["_screen", objNull, [objNull]], ["_video", "", [""]], ["_selection", 0, [0]], ["_audio", [], [[]]]];
private _skipVariable = generateUid;
_screen setObjectTexture [_selection, _video];
_screen say3D _audio;
[_video, [10, 10], [1, 1, 1, 1], _skipVariable] call BIS_fnc_playVideo;
[missionNamespace, _skipVariable, clientOwner];