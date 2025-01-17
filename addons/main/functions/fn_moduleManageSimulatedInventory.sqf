isNil {
	params [["_logic", objNull], ["_units", []], ["_activated", false]];

	if _activated then {
		{
			
			[
				_x, 
				[
					[
						_logic getVariable ["KH_ModuleManageSimulatedInventoryName", ""], 
						_logic getVariable ["KH_ModuleManageSimulatedInventoryDescription", ""], 
						parseNumber (_logic getVariable ["KH_ModuleManageSimulatedInventoryAmount", "0"]),
						_logic getVariable ["KH_ModuleManageSimulatedInventoryForceAmount", false]
					]
				]
			] call KH_fnc_manageSimulatedInventory;
		} forEach _units;
	};
};

true;