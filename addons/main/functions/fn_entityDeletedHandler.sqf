params [["_entity", objNull, [objNull]]];

if (_entity in KH_var_allEntities) then {
    KH_var_allEntities deleteAt (KH_var_allEntities find _entity);
};

if (_entity in KH_var_allLocalEntities) then {
    KH_var_allLocalEntities deleteAt (KH_var_allLocalEntities find _entity);
};

if (_entity in KH_var_allMen) then {
    KH_var_allMen deleteAt (KH_var_allMen find _entity);
};

if (_entity in KH_var_allLocalMen) then {
    KH_var_allLocalMen deleteAt (KH_var_allLocalMen find _entity);
};