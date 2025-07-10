params [["_types", ["PLAYERS"]], ["_dead", true]];
private _clients = [];

{
	switch (typeName _x) do {
		case "SIDE": {
			private _side = _x;

			{
				if (isPlayer _x) then {
					if ((side (group _x)) == _side) then {
						_clients pushBackUnique _x;
					};
				};
			} forEach allUnits;

			if _dead then {
				{
					if !(isAgent (teamMember _x)) then {
						if (isPlayer _x) then {
							if ((side (group _x)) == _side) then {
								_clients pushBackUnique _x;
							};
						};
					};
				} forEach allDeadMen;
			};
		};

		case "GROUP": {
			private _group = _x;

			{
				if (isPlayer _x) then {
					if ((group _x) == _group) then {
						_clients pushBackUnique _x;
					};
				};
			} forEach allUnits;

			if _dead then {
				{
					if !(isAgent (teamMember _x)) then {
						if (isPlayer _x) then {
							if ((group _x) == _group) then {
								_clients pushBackUnique _x;
							};
						};
					};
				} forEach allDeadMen;
			};
		};

		case "STRING": {
			switch _x do {
				case "CURATORS": {
					{
						private _curatorUnit = getAssignedCuratorUnit _x;

						if (!(isNull _curatorUnit) && (isPlayer _curatorUnit)) then {
							_clients pushBackUnique _curatorUnit;
						};
					} forEach allCurators;
				};

				case "HEADLESS": {
					{
						if (isPlayer _x) then {
							_clients pushBackUnique _x;
						};
					} forEach (entities [["HeadlessClient_F"], [], true, true]);
				};

				case "VIRTUAL_SPECTATORS": {
					{
						if (isPlayer _x) then {
							_clients pushBackUnique _x;
						};
					} forEach (entities [["VirtualSpectator_F"], [], true, true]);
				};

				case "ACE_SPECTATORS": {
					{
						if (isPlayer _x) then {
							_clients pushBackUnique _x;
						};
					} forEach (entities [["ace_spectator_virtual"], [], true, true]);
				};

				case "PLAYERS": {
					{
						if _dead then {
							_clients pushBackUnique _x;
						}
						else {
							if (alive _x) then {
								_clients pushBackUnique _x;
							};
						};
					} forEach KH_var_allPlayerUnits;
				};

				case "ALL": {
					{
						if (isPlayer _x) then {
							_clients pushBackUnique _x;
						};
					} forEach allUnits;

					{
						if (isPlayer _x) then {
							_clients pushBackUnique _x;
						};
					} forEach (entities [["HeadlessClient_F", "VirtualSpectator_F", "ace_spectator_virtual"], [], true, true]);
					
					if _dead then {
						{
							if !(isAgent (teamMember _x)) then {
								if (isPlayer _x) then {
									_clients pushBackUnique _x;
								};
							};
						} forEach allDeadMen;
					};
				};
			};
		};
	};
} forEach _types;

_clients;