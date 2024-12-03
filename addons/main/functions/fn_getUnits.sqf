params [["_types", ["REGULAR", "PLAYABLE"]], ["_players", true], ["_dead", false]];
private _units = [];

{
	switch true do {
		case (_x isEqualType grpNull): {
			private _group = _x;
			
			{
				if ((group _x) == _group) then {
					if _players then {
						_units pushBackUnique _x;
					}
					else {
						if !(isPlayer _x) then {
							_units pushBackUnique _x;
						};
					};
				};
			} forEach allUnits;

			if _dead then {
				{
					if !(isAgent (teamMember _x)) then {
						if ((group _x) == _group) then {
							if _players then {
								_units pushBackUnique _x;
							}
							else {
								if !(isPlayer _x) then {
									_units pushBackUnique _x;
								};
							};
						};
					};
				} forEach allDeadMen;
			};		
		};
		
		case (_x isEqualType sideUnknown): {
			private _side = _x;
			
			{
				if ((side (group _x)) == _side) then {
					if _players then {
						_units pushBackUnique _x;
					}
					else {
						if !(isPlayer _x) then {
							_units pushBackUnique _x;
						};
					};
				};
			} forEach allUnits;

			if _dead then {
				{
					if !(isAgent (teamMember _x)) then {
						if ((side (group _x)) == _side) then {
							if _players then {
								_units pushBackUnique _x;
							}
							else {
								if !(isPlayer _x) then {
									_units pushBackUnique _x;
								};
							};
						};
					};
				} forEach allDeadMen;
			};

			{
				if !((agent _x) isKindOf "Animal") then {
					if _dead then {
						if _players then {
							_units pushBackUnique (agent _x);
						}
						else {
							if !(isPlayer (agent _x)) then {
								_units pushBackUnique (agent _x);
							};
						};
					}
					else {
						if (alive (agent _x)) then {
							if _players then {
								_units pushBackUnique (agent _x);
							}
							else {
								if !(isPlayer (agent _x)) then {
									_units pushBackUnique (agent _x);
								};
							};
						};
					};
				};
			} forEach agents;
		};

		case (_x isEqualType ""): {
			switch true do {
				case (_x == "PLAYABLE"): {
					{
						if _players then {
							_units pushBackUnique _x;
						}
						else {
							if !(isPlayer _x) then {
								_units pushBackUnique _x;
							};
						};
					} forEach playableUnits;

					if (_players && _dead) then {
						{
							if !(isAgent (teamMember _x)) then {
								if (isPlayer _x) then {
									_units pushBackUnique _x;
								};
							};
						} forEach allDeadMen;
					};
				};

				case (_x == "REGULAR"): {
					{
						if _players then {
							if ((isPlayer _x) || (!(isPlayer _x) && !(_x in playableUnits))) then {
								_units pushBackUnique _x;
							};
						}
						else {
							if (!(isPlayer _x) && !(_x in playableUnits)) then {
								_units pushBackUnique _x;
							};
						};
					} forEach allUnits;

					if _dead then {
						{
							if !(isAgent (teamMember _x)) then {
								if _players then {
									_units pushBackUnique _x;
								}
								else {
									if !(isPlayer _x) then {
										_units pushBackUnique _x;
									};
								};
							};
						} forEach allDeadMen;
					};
				};

				case (_x == "AGENTS"): {
					{
						if !((agent _x) isKindOf "Animal") then {
							if _dead then {
								if _players then {
									_units pushBackUnique (agent _x);
								}
								else {
									if !(isPlayer (agent _x)) then {
										_units pushBackUnique (agent _x);
									};
								};
							}
							else {
								if (alive (agent _x)) then {
									if _players then {
										_units pushBackUnique (agent _x);
									}
									else {
										if !(isPlayer (agent _x)) then {
											_units pushBackUnique (agent _x);
										};
									};
								};
							};
						};
					} forEach agents;
				};

				case (_x == "ALL"): {
					{
						if !((agent _x) isKindOf "Animal") then {
							if _dead then {
								if _players then {
									_units pushBackUnique (agent _x);
								}
								else {
									if !(isPlayer (agent _x)) then {
										_units pushBackUnique (agent _x);
									};
								};
							}
							else {
								if (alive (agent _x)) then {
									if _players then {
										_units pushBackUnique (agent _x);
									}
									else {
										if !(isPlayer (agent _x)) then {
											_units pushBackUnique (agent _x);
										};
									};
								};
							};
						};
					} forEach agents;

					{
						if _players then {
							_units pushBackUnique _x;
						}
						else {
							if !(isPlayer _x) then {
								_units pushBackUnique _x;
							};
						};
					} forEach allUnits;

					if _dead then {
						{
							if !(isAgent (teamMember _x)) then {
								if _players then {
									_units pushBackUnique _x;
								}
								else {
									if !(isPlayer _x) then {
										_units pushBackUnique _x;
									};
								};
							};
						} forEach allDeadMen;
					};
				};
			};
		};
	};
} forEach _types;

_units;