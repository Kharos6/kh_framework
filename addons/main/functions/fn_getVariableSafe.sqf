params [
    ["_namespace", missionNamespace, [objNull, teamMemberNull, grpNull, locationNull, missionNamespace, taskNull, controlNull, displayNull]],
    ["_name", "", [""]],
    "_defaultValue" 
];

if (_namespace isNil _name) then {
    _namespace setVariable [_name, _defaultValue];
    _defaultValue;
}
else {
    _namespace getVariable _name;
};