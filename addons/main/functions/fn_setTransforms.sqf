params ["_entity", "_position", "_rotation"];
[_entity, _rotation select 0, _rotation select 1] call KH_fnc_setRotation;
[_entity, _position select 0, _position select 1, _position select 2] call KH_fnc_setPosition;