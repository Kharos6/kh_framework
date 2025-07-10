params ["_entity", "_position", ["_positionType", "AEL"], ["_relative", false]];

switch (typeName _position) do {
	case "ARRAY": {
		switch _positionType do {
			case "ATL": {
				if !_relative then {
					_entity setPosATL _position;
					getPosATL _entity;
				}
				else {
					_entity setPosATL ((getPosATL _entity) vectorAdd _position);
					getPosATL _entity;
				};
			};

			case "ASL": {
				if !_relative then {
					_entity setPosASL _position;
					getPosASL _entity;
				}
				else {
					_entity setPosASL ((getPosASL _entity) vectorAdd _position);
					getPosASL _entity;
				};
			};

			case "ASLW": {
				if !_relative then {
					_entity setPosASLW _position;
					getPosASLW _entity;
				}
				else {
					_entity setPosASLW ((getPosASLW _entity) vectorAdd _position);
					getPosASLW _entity;
				};
			};

			case "AEL": {		
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

			case "AGL": {		
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

			case "AGLS": {
				if !_relative then {
					_entity setPos _position;
					getPos _entity;
				}
				else {
					_entity setPos ((getPos _entity) vectorAdd _position);
					getPos _entity;
				};
			};

			case "WORLD": {
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

	case "OBJECT": {
		switch _positionType do {
			case "ATL": {
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

			case "ASL": {
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

			case "ASLW": {
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

			case "AEL": {		
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

			case "AGL": {		
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

			case "AGLS": {
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

			case "WORLD": {
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