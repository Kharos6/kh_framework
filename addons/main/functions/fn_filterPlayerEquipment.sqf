params [["_equipment", [], [[]]], ["_overwrite", true, [true]], ["_filter", true, [true]]];
KH_var_filterPlayerEquipmentFilter = _filter;

if (isNil "KH_var_filterPlayerEquipmentList") then {
    KH_var_filterPlayerEquipmentList = [];
};

_equipment = _equipment apply {toLowerANSI _x;};

if _overwrite then {
    KH_var_filterPlayerEquipmentList = _equipment;
    nil;
}
else {
    KH_var_filterPlayerEquipmentList insert [-1, _equipment, true];
};