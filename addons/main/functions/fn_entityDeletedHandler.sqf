params [["_entity", objNull, [objNull]]];

if (_entity in KH_var_allLocalEntities) then {
    KH_var_allLocalEntities deleteAt (KH_var_allLocalEntities find _entity);
};