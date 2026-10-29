params [["_entity", objNull, [objNull]]];
private _netId = netId _entity;

if (_netId isEqualTo "0:0") then {
    "LOCAL";
}
else {
    ["GLOBAL", "TERRAIN"] select (((parseNumber (_netId select [0, 1])) isEqualTo 1) && ((parseNumber (_netId select [2, 2])) < 0));
};