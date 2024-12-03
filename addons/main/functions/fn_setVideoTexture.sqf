params ["_screen", "_video", "_texture", ["_audio", []]];
_screen setObjectTexture [_texture, _video];
_screen say3D _audio;
[_video, [10, 10]] spawn BIS_fnc_playVideo;
true;