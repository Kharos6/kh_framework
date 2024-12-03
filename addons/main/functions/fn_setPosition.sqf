params ["_entity", "_position", ["_positionType", "AEL"], ["_relative", false]];

switch true do {
	case (_position isEqualType []): {
		switch true do {
			case (_positionType == "ATL"): {
				if !_relative then {
					_entity setPosATL _position;
					getPosATL _entity;
				}
				else {
					_entity setPosATL ((getPosATL _entity) vectorAdd _position);
					getPosATL _entity;
				};
			};

			case (_positionType == "ASL"): {
				if !_relative then {
					_entity setPosASL _position;
					getPosASL _entity;
				}
				else {
					_entity setPosASL ((getPosASL _entity) vectorAdd _position);
					getPosASL _entity;
				};
			};

			case (_positionType == "ASLW"): {
				if !_relative then {
					_entity setPosASLW _position;
					getPosASLW _entity;
				}
				else {
					_entity setPosASLW ((getPosASLW _entity) vectorAdd _position);
					getPosASLW _entity;
				};
			};

			case (_positionType == "AEL"): {		
				if (surfaceIsWater _position) then {
					if !_relative then {
						_entity setPosASL _position;
						getPosASL _entity;
					}
					else {
						_entity setPosASL ((getPosASL _entity) vectorAdd _position);
						getPosASL _entity;
					};
				}
				else {
					if !_relative then {
						_entity setPosATL _position;
						getPosATL _entity;
					}
					else {
						_entity setPosATL ((getPosATL _entity) vectorAdd _position);
						getPosATL _entity;
					};
				};
			};

			case (_positionType == "AGL"): {		
				if (surfaceIsWater _position) then {
					if !_relative then {
						_entity setPosASLW _position;
						getPosASLW _entity;
					}
					else {
						_entity setPosASLW ((getPosASLW _entity) vectorAdd _position);
						getPosASLW _entity;
					};
				}
				else {
					if !_relative then {
						_entity setPosATL _position;
						getPosATL _entity;
					}
					else {
						_entity setPosATL ((getPosATL _entity) vectorAdd _position);
						getPosATL _entity;
					};
				};
			};

			case (_positionType == "AGLS"): {
				if !_relative then {
					_entity setPos _position;
					getPos _entity;
				}
				else {
					_entity setPos ((getPos _entity) vectorAdd _position);
					getPos _entity;
				};
			};

			case (_positionType == "WORLD"): {
				if !_relative then {
					_entity setPosWorld _position;
					getPosWorld _entity;
				}
				else {
					_entity setPosWorld ((getPosWorld _entity) vectorAdd _position);
					getPosWorld _entity;
				};
			};

			default {
				[];
			};
		};
	};

	case (_position isEqualType objNull): {
		switch true do {
			case (_positionType == "ATL"): {
				private _entityPosition = getPosATL _position;

				if !_relative then {
					_entity setPosATL _entityPosition;
					getPosATL _entity;
				}
				else {
					_entity setPosATL ((getPosATL _entity) vectorAdd _entityPosition);
					getPosATL _entity;
				};
			};

			case (_positionType == "ASL"): {
				private _entityPosition = getPosASL _position;

				if !_relative then {
					_entity setPosASL _entityPosition;
					getPosASL _entity;
				}
				else {
					_entity setPosASL ((getPosASL _entity) vectorAdd _entityPosition);
					getPosASL _entity;
				};
			};

			case (_positionType == "ASLW"): {
				private _entityPosition = getPosASLW _position;

				if !_relative then {
					_entity setPosASLW _entityPosition;
					getPosASLW _entity;
				}
				else {
					_entity setPosASLW ((getPosASLW _entity) vectorAdd _entityPosition);
					getPosASLW _entity;
				};
			};

			case (_positionType == "AEL"): {		
				private _entityPosition = getPosWorld _position;

				if (surfaceIsWater _entityPosition) then {
					if !_relative then {
						_entity setPosASL _entityPosition;
						getPosASL _entity;
					}
					else {
						_entity setPosASL ((getPosASL _entity) vectorAdd _entityPosition);
						getPosASL _entity;
					};
				}
				else {
					if !_relative then {
						_entity setPosATL _entityPosition;
						getPosATL _entity;
					}
					else {
						_entity setPosATL ((getPosATL _entity) vectorAdd _entityPosition);
						getPosATL _entity;
					};
				};
			};

			case (_positionType == "AGL"): {		
				private _entityPosition = getPosWorld _position;

				if (surfaceIsWater _entityPosition) then {
					if !_relative then {
						_entity setPosASLW _entityPosition;
						getPosASLW _entity;
					}
					else {
						_entity setPosASLW ((getPosASLW _entity) vectorAdd _entityPosition);
						getPosASLW _entity;
					};
				}
				else {
					if !_relative then {
						_entity setPosATL _entityPosition;
						getPosATL _entity;
					}
					else {
						_entity setPosATL ((getPosATL _entity) vectorAdd _entityPosition);
						getPosATL _entity;
					};
				};
			};

			case (_positionType == "AGLS"): {
				private _entityPosition = getPos _position;

				if !_relative then {
					_entity setPos _entityPosition;
					getPos _entity;
				}
				else {
					_entity setPos ((getPos _entity) vectorAdd _entityPosition);
					getPos _entity;
				};
			};

			case (_positionType == "WORLD"): {
				private _entityPosition = getPosWorld _position;
				
				if !_relative then {
					_entity setPosWorld _entityPosition;
					getPosWorld _entity;
				}
				else {
					_entity setPosWorld ((getPosWorld _entity) vectorAdd _entityPosition);
					getPosWorld _entity;
				};
			};

			default {
				[];
			};
		};
	};

	default {
		[];
	};
};