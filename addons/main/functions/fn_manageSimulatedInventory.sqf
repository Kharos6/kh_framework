params ["_entity", "_items"];
private _previousInventory = _entity getVariable ["KH_var_simulatedInventoryItems", createHashMap];

[
	[_entity, _items],
	{
		params ["_entity", "_items"];
		private _currentInventory = _entity getVariable ["KH_var_simulatedInventoryItems", createHashMap];

		{
			_x params ["_name", ["_description", ""], ["_amount", 1], ["_forceAmount", false]];
			_amount = floor _amount;
			private _currentItem = _currentInventory get _name;
			
			if (isNil "_currentItem") then {
				_currentInventory insert [[_name, [_description, _amount max 0]]];
			}
			else {
				if (_description == "") then {
					_description = _currentItem select 0;
				};
				
				if _forceAmount then {
					_currentInventory set [_name, [_description, _amount], false];
				}
				else {
					_currentInventory set [_name, [_description, (_currentItem select 1) + _amount], false];
				};
			};

			_currentItem = _currentInventory get _name;

			if !(isNil "_currentItem") then {
				if ((_currentItem select 1) == 0) then {
					_currentInventory deleteAt _name;
				};
			};
		} forEach _items;

		_entity setVariable ["KH_var_simulatedInventoryItems", _currentInventory, true];
	},
	"SERVER",
	true
] call KH_fnc_execute;

_previousInventory;