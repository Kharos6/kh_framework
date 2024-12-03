params ["_entities", "_distance"];
private _id = [missionNamespace, "KH_var_dynamicSimulation", "ACTIVE", false] call KH_fnc_atomicVariable;

[
	{
		private _id = _args select 2;
		private _idState = missionNamespace getVariable [_id, "ACTIVE"];

		if (idState != "INACTIVE") then {
			private _entities = _args select 1;

			switch true do {
				case (_idState == "ACTIVE"): {
					private _distance = _args select 0;
					
					{
						private _object = _x;
						
						{
							if ((_x distance _object) <= (_distance)) then {
								if !(simulationEnabled _object) then {
									_object enableSimulationGlobal true;
								};
								
								break;
							}
							else {
								if ((speed _object) == 0) then {
									if (simulationEnabled _object) then {
										_object enableSimulationGlobal false;
									};
								}
								else {
									if !(simulationEnabled _object) then {
										_object enableSimulationGlobal true;
									};
								};
							};
						} forEach KH_var_allPlayerUnits;
					} forEach _entities;
				};

				case (_idState == "TERMINATE"): {
					{
						if !(simulationEnabled _x) then {
							_x enableSimulationGlobal true;
						};
					} forEach _entities;

					[_handle] call CBA_fnc_removePerFrameHandler;
				};		
			};
		};
	}, 
	0.1, 
	[_distance, _entities, _id]
] call CBA_fnc_addPerFrameHandler;

["PRIVATE_HANDLER", _id, clientOwner];