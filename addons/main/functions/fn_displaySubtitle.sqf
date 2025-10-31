params [["_speaker", "", [""]], ["_text", "", [""]], ["_duration", 0, [0]]];

if (_duration isEqualTo 0) then {
	_duration = ((count _text) / 1125) * 60;
};

if !(isNull (uiNamespace getVariable ["KH_var_subtitle", controlNull])) then {
	ctrlDelete (uiNamespace getVariable ["KH_var_subtitle", controlNull]);
};

uiNamespace setVariable ["KH_var_subtitle", controlNull];
private _control = (findDisplay 46) ctrlCreate ["RscStructuredText",-1];
_control ctrlSetBackgroundColor (["Subtitles", "Background"] call BIS_fnc_displayColorGet);
_control ctrlSetTextColor (["Subtitles", "Text"] call BIS_fnc_displayColorGet);
_control ctrlSetPosition [0.5 - (0.4 * safeZoneW) / 2, safeZoneY + (7/8) * safeZoneH, 0.4 * safeZoneW, safeZoneH];
_control ctrlCommit 0;
_control ctrlSetStructuredText (parseText (format ["<t align='center' shadow='2' size='%3' font='RobotoCondensedBold'>%1:<br/>%2</t>", _speaker, _text, (safeZoneH * 0.65) max 1]));
private _textHeight = ctrlTextHeight _control;
_control ctrlSetPosition [0.5 - (0.4 * safeZoneW) / 2, (safeZoneY + (7/8) * safeZoneH) - _textHeight, 0.4 * safeZoneW, _textHeight];
_control ctrlCommit 0;
private _toggleState = true;
uiNamespace setVariable ["KH_var_subtitle", _control];

[
	[_control],
	{
		params ["_control"];

		if !(isNull _control) then {
			ctrlDelete _control;
		};
	}, 
	true,
	str _duration,
	false
] call KH_fnc_execute;

_control;