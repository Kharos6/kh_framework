params ["_entity", "_item"];
private _map = _entity getVariable ["KH_var_simulatedInventoryItems", createHashMap];
private _itemProperties = _map get _item;

if !(isNil "_itemProperties") then {
	_itemProperties select 1;
}
else {
	0;
};