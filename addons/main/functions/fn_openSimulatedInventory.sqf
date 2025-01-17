params ["_entity"];

if (isNull (objectParent player)) then {
	player playAction "Gear";
};

private _display = (findDisplay 46) createDisplay "KH_ResourceSimulatedInventory";
KH_var_currentInteractionTarget = _entity;
private _mainControl = _display displayCtrl 100;
private _entityName = "";

if (_entity isKindOf "Man") then {
	_entityName = name _entity;
}
else {
	_entityName = typeOf _entity;
};

(_display displayCtrl 101) ctrlSetText (format ["%1 INVENTORY", toUpper _entityName]);
KH_var_currentSimulatedInventoryItems = [];

[
	{
		_args params ["_entity", "_display"];

		if !(isNull _display) then {
			private _list = _entity getVariable ["KH_var_simulatedInventoryItems", createHashMap];

			if ((_list toArray true) isNotEqualTo KH_var_currentSimulatedInventoryItems) then {
				private _mainControl = _display displayCtrl 100;
				lbClear _mainControl;

				{
					_y params ["_description", "_amount"];
					private _index = _mainControl lbAdd (format ["%1x %2", _amount, _x]);
					_mainControl lbSetTooltip [_index, _description];
				} forEach _list;

				KH_var_currentSimulatedInventoryItems = _list toArray true;
			};
		}
		else {
			[_handle] call CBA_fnc_removePerFrameHandler;
		};
	},
	0, 
	[_entity, _display]
] call CBA_fnc_addPerFrameHandler;

[
	["CONTROL", _mainControl],
	"LBSelChanged",
	[_entity, _display, _mainControl],
	{
		_args params ["_entity", "_display", "_mainControl"];
		private _positionX = ((ctrlPosition _mainControl) select 0) + 0.64;
		private _positionY = (ctrlPosition _mainControl) select 1;
		private _itemName = _mainControl lbText (lbCurSel _mainControl);
		KH_var_currentSelectedSimulatedInventoryItem = _itemName select [(_itemName find " ") + 1];
		KH_var_nearbySimulatedInventoryTargets = ((player nearEntities [["Man", "Air", "Car", "Motorcycle", "Tank"], 4]) - [_entity]);
		private _options = [];

		{
			private _receiverName = "";

			if (_x isKindOf "Man") then {
				_receiverName = name _x;
			}
			else {
				_receiverName = typeOf _x;
			};

			_options pushBack [
				format ["Transfer to %1", _receiverName], 
				"Transfer one of this item from this inventory to the selected entity, as long as the entity remains within 4 meters.",
				{
					((([KH_var_currentInteractionTarget, KH_var_currentSelectedSimulatedInventoryItem] call KH_fnc_getSimulatedInventoryItemCount) > 0) && ({_x in (player nearEntities [["Man", "Air", "Car", "Motorcycle", "Tank"], 4])} forEach KH_var_nearbySimulatedInventoryTargets));
				}, 
				[_x, _entity],
				{
					params ["_receiver", "_giver"];

					if (([KH_var_currentInteractionTarget, KH_var_currentSelectedSimulatedInventoryItem] call KH_fnc_getSimulatedInventoryItemCount) > 0) then {
						[_receiver, [[KH_var_currentSelectedSimulatedInventoryItem, "", 1, false]]] call KH_fnc_manageSimulatedInventory;
						[_giver, [[KH_var_currentSelectedSimulatedInventoryItem, "", -1, false]]] call KH_fnc_manageSimulatedInventory;
					};
				}
			];
		} forEach KH_var_nearbySimulatedInventoryTargets;

		[_display, _positionX, _positionY, _options] call KH_fnc_contextMenu;
	}
] call KH_fnc_addEventHandler;

[
	["DISPLAY", _display],
	"KeyDown",
	[_display],
	{
		_args params ["_display"];
		private _key = _this select 1;

		if (_key isEqualTo 0xDC) then {
			_display closeDisplay 2;
		};

		nil;
	}
] call KH_fnc_addEventHandler;

_display;