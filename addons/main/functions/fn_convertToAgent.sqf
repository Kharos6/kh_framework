params [["_unit", objNull, [objNull]], ["_init", {}, [{}]]];
private _attributes = [_unit] call KH_fnc_getUnitAttributes;
private _agent = createAgent [_attributes select 3, _unit, [], 0, "CAN_COLLIDE"];
deleteVehicle _unit;
[agent _agent, _attributes, [], false] call KH_fnc_setUnitAttributes;
[_agent] call _init;
_agent;