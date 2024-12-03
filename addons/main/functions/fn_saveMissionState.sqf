params ["_identifier", ["_objects", entities [[], ["Man", "Logic", "Animal"], false, false]], ["_players", [["PLAYERS"], true] call KH_fnc_getClients], ["_units", [["REGULAR", "PLAYABLE"], false, false] call KH_fnc_getUnits], ["_agents", [["AGENTS"], false, false] call KH_fnc_getUnits], ["_variables", []]];
private _variableIterator = 0;
private _savedObjects = [];
private _cargoObjects = [];
private _savedUnits = createHashMap;
private _savedAgents = [];
private _worldInfo = [date, overcast, fog, rain, lightnings, waves, wind];

{
	private _variableName = vehicleVarName _x;
	
	if (_variableName == "") then {
		_x setVehicleVarName format ["KH_var_savedUnit_%1_%2", _identifier, _variableIterator];
		_variableIterator = _variableIterator + 1;
	};
	
	private _attributes = [_x] call KH_fnc_getUnitAttributes;
	private _unitGroup = groupId (group _x);
	private _savedUnit = [_unitGroup, [_attributes]];
	private _currentGroupUnits = _savedUnits get _unitGroup;
	
	if (isNil "_currentGroupUnits") then {
		_savedUnits insert _savedUnit;
	}
	else {
		_currentGroupUnits pushBack (_savedUnit select 1);
		_savedUnits set [_unitGroup, _currentGroupUnits];
	};
} forEach _units;

{	
	_savedAgents pushBack ([_x] call KH_fnc_getUnitAttributes);
} forEach _agents;

_variableIterator = 0;

{
	private _className = typeOf _x;
	private _variableName = vehicleVarName _x;
	private _position = [_x, "AGL", []] call KH_fnc_getPosition;
	private _vectorDir = vectorDir _x;
	private _vectorUp = vectorUp _x;
	private _damage = damage _x;
	private _hitpointDamage = getAllHitPointsDamage _x;
	private _fuel = fuel _x;
	private _ammoCargo = getAmmoCargo _x;
	private _fuelCargo = getFuelCargo _x;
	private _repairCargo = getRepairCargo _x;
	private _simulationState = simulationEnabled _x;
	private _dynamicSimulationState = dynamicSimulationEnabled _x;
	private _isSimple = isSimpleObject _x;
	private _crew = str (crew _x);
	private _transportBackpack = getNumber (configFile >> "CfgVehicles" >> _className >> "transportmaxbackpacks");
	private _transportMagazines = getNumber (configFile >> "CfgVehicles" >> _className >> "transportmaxmagazines");
	private _transportWeapons = getNumber (configFile >> "CfgVehicles" >> _className >> "transportmaxweapons");
	
	if (((_transportBackpack > 0) || (_transportMagazines > 0) || (_transportWeapons > 0)) || ((_x getVariable ["ace_cargo_loaded", []]) isNotEqualTo [])) then {
		if (_variableName == "") then {
			_x setVehicleVarName format ["KH_var_savedObject_%1_%2", _identifier, _variableIterator];
			_variableIterator = _variableIterator + 1;
			_variableName = vehicleVarName _x;
		};
		
		_cargoObjects pushBack _x;
	};
	
	_savedObjects pushBack [_className, _variableName, _position, _vectorDir, _vectorUp, _damage, _hitpointDamage, _fuel, _ammoCargo, _fuelCargo, _repairCargo, simulationState, _dynamicSimulationState, _isSimple, _crew];
} forEach _objects;

private _cargoObjectInventories = [_identifier, _cargoObjects] call KH_fnc_saveCargoInventories;
private _playerLoadouts = [_identifier, _players] call KH_fnc_savePlayerLoadouts;
private _stateEntries = [_savedObjects, _cargoObjectInventories, _playerLoadouts, _savedUnits toArray false, _savedAgents, _worldInfo];

[
	[_identifier, _stateEntries, _variables], 
	{
		params ["_identifier", "_stateEntries", "_variables"];
		[format ["KH_var_savedMissionState_%1", _identifier], _stateEntries] call KH_fnc_editMissionProfileNamespace;

		if (_variables isNotEqualTo []) then {
			{
				[_x select 0, _x select 1] call KH_fnc_editMissionProfileNamespace;
			} forEach _variables;
		};
	},
	"GLOBAL",
	"THIS_FRAME"
] call KH_fnc_execute;

_stateEntries;