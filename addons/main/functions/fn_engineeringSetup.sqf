params [
    ["_vehicle", objNull, [objNull]], 
    ["_state", true, [true]], 
    ["_damageMultiplier", 1, [0]],
    ["_totalDamageMultiplier", 1, [0]],
    ["_impactDamageMultiplier", 1, [0]], 
    ["_hitPointDamageMultipliers", [], [[], createHashMap]], 
    ["_recoverable", true, [true]]
];

private _continue = _vehicle isNil "KH_var_engineeringHandling"; 
_vehicle setVariable ["KH_var_engineeringHandling", _state, true];
_vehicle setVariable ["KH_var_engineeringDamageMultiplier", _damageMultiplier, true];
_vehicle setVariable ["KH_var_engineeringTotalDamageMultiplier", _totalDamageMultiplier, true];
_vehicle setVariable ["KH_var_engineeringImpactDamageMultiplier", _impactDamageMultiplier, true];

if (_hitPointDamageMultipliers isEqualType []) then {
    _hitPointDamageMultipliers = createHashMapFromArray _hitPointDamageMultipliers;
};

_vehicle setVariable ["KH_var_engineeringHitPointDamageMultipliers", _hitPointDamageMultipliers, true];
_vehicle setVariable ["KH_var_engineeringRecoverable", _recoverable, true];

if _continue then {
    [
        ["ENTITY", _vehicle, "PERSISTENT"],
        "HandleDamage",
        [],
        {
            params ["_unit", "_selection", "_damage", "_source", "_projectile", "_hitPartIndex", "_instigator", "_hitPoint", "_directHit", "_context"];
            if !(_unit getVariable ["KH_var_engineeringHandling", false]) exitWith {};
            _hitPoint = toLowerANSI _hitPoint;

            if ((_context isEqualTo 0) && (_projectile isEqualTo "") && (isNull _source) && (isNull _instigator)) exitWith {
                _damage;
            };

            private _totalDamage = _context isEqualTo 0;

            private _currentDamage = if _totalDamage then {
                damage _unit;
            } 
            else {
                _unit getHitIndex _hitPartIndex;
            };

            _damage = (_damage - _currentDamage) max 0;

            if (_projectile isEqualTo "") then {
                _damage = _damage * (_unit getVariable ["KH_var_engineeringImpactDamageMultiplier", 1]);
            };

            private _hitPointDamageMultiplier = (_unit getVariable ["KH_var_engineeringHitPointDamageMultipliers", createHashMap]) get _hitPoint;
            
            if !_totalDamage then {
                (_currentDamage + (_damage * ([1, _hitPointDamageMultiplier] select !(isNil "_hitPointDamageMultiplier")) * (_unit getVariable ["KH_var_engineeringDamageMultiplier", 1]))) min ([1, 0.75] select ((_unit getVariable ["KH_var_engineeringRecoverable", false]) && (_hitPoint isEqualTo "hithull")));
            }
            else {
                (_currentDamage + (_damage * (_unit getVariable ["KH_var_engineeringTotalDamageMultiplier", 1]) * (_unit getVariable ["KH_var_engineeringDamageMultiplier", 1]))) min ([1, 0.75] select (_unit getVariable ["KH_var_engineeringRecoverable", false]));
            };
        }
    ] call KH_fnc_addEventHandler;
};

nil;