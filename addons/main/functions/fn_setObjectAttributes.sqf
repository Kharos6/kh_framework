params [["_object", objNull, [objNull]], ["_attributes", [], [[]]], ["_overrideAttributes", [], [[]]], ["_respectSimple", true, [true]]];
(_attributes select [3]) params ["_type", "_simple", "_savedAttributes"];

if _respectSimple then {
	if _simple then {
		if !(isSimpleObject _object) then {
			deleteVehicle _object;
			_object = createSimpleObject [_type, [0, 0, 0], false];
		};
	}
	else {
		if (isSimpleObject _object) then {
			deleteVehicle _object;
			_object = createVehicle [_type, [0, 0, 0], [], 0, "CAN_COLLIDE"];
		};
	};
};

if (_overrideAttributes isNotEqualTo []) then {
	private _currentAttributes = ([_object] call KH_fnc_getObjectAttributes) select 5;

	{
		if (_x isEqualType 0) then {
			_savedAttributes set [_x, _currentAttributes select _x];
		}
		else {
			_x params [["_index", 0, [0]], "_value"];
			_savedAttributes set [_index, _value];
		};
	} forEach _overrideAttributes;
};

_savedAttributes params [
	"_simulation",
	"_dynamicSimulation",
	"_hidden",
	"_damageAllowed",
	"_position",
	"_vectorDir",
	"_vectorUp",
	"_velocityModelSpace",
    "_angularVelocityModelSpace",
	"_damage",
	"_hitPointsDamage",
    "_locked",
    "_lockedInventory",
    "_inventory",
    "_engineOn",
    "_light",
    "_collisionLight",
    "_allowedService",
    "_equipmentDisabled",
    "_loadout",
    "_fuelConsumptionCoef",
    "_waterLeakiness",
    "_waterFillPercentage",
    "_vehicleCargo",
	"_collisionDisabledWith",
    "_variables"
];

[
	[
        _object,
		_simulation,
        _hidden,
        _inventory,
        _equipmentDisabled,
        _vehicleCargo
	], 
	{
		params [
            "_object",
            "_simulation",
            "_hidden",
            "_inventory",
            "_equipmentDisabled",
            "_vehicleCargo"
		];

        _object enableSimulationGlobal _simulation;
        _object hideObjectGlobal _hidden;
        _object setPosATL _position;
        [_object, _inventory] call KH_fnc_setObjectInventory;
        _equipmentDisabled params ["_nvg", "_ti"];
        _object disableNVGEquipment _nvg;
        _object disableTIEquipment _ti;
        _vehicleCargo setVehicleCargo _object;
	}, 
	"SERVER", 
	true, 
	false
] call KH_fnc_execute;

[
	[
		_object,
        _damageAllowed,
        _vectorDir,
        _vectorUp,
        _velocityModelSpace,
        _angularVelocityModelSpace,
        _damage,
        _hitPointsDamage,
        _locked,
        _engineOn,
        _light,
        _collisionLight,
        _allowedService,
        _loadout,
        _fuelConsumptionCoef,
        _waterLeakiness,
        _collisionDisabledWith,
        _variables
	], 
	{
		params [
			"_object",
            "_damageAllowed",
            "_vectorDir",
            "_vectorUp",
            "_velocityModelSpace",
            "_angularVelocityModelSpace",
            "_damage",
            "_hitPointsDamage",
            "_locked",
            "_engineOn",
            "_light",
            "_collisionLight",
            "_allowedService",
            "_loadout",
            "_fuelConsumptionCoef",
            "_waterLeakiness",
            "_collisionDisabledWith",
            "_variables"
		];

        _object allowDamage _damageAllowed;
        _object setVectorDirAndUp [_vectorDir, _vectorUp];
        _object setVelocityModelSpace _velocityModelSpace;
        _object setAngularVelocityModelSpace _angularVelocityModelSpace;
        _object setDamage _damage;
		private _hitPointNames = _hitPointsDamage select 0;
		private _hitPointValues = _hitPointsDamage select 2;

		for "_i" from 0 to ((count _hitPointNames) - 1) do {
			_object setHitPointDamage [_hitPointNames select _i, _hitPointValues select _i];
		};

        _object lock _locked;
        _object engineOn _engineOn;
        _object setPilotLight _light;
        _object setCollisionLight _collisionLight;
        _object allowService _allowedService;

        {
            _object setAmmo [_x select 3, _x select 6];
        } forEach _weaponsInfo;

        {
            _object setPylonLoadout [_x select 0, _x select 3, true];
        } forEach _pylonsInfo;

        {
            _object setAmmoOnPylon _x;
        } forEach _pylonsAmmo;

        [_object, _loadout] call KH_fnc_setVehicleLoadout;
        _object setFuelConsumptionCoef _fuelConsumptionCoef;
        _object setWaterLeakiness _waterLeakiness;
		private _collisionEntity = ["OBJECT", _collisionDisabledWith] call KH_fnc_getEntityByIdentifier;

		if !(isNull _collisionEntity) then {
			_object disableCollisionWith _collisionEntity;
		};

		{
			_object setVariable _x;
		} forEach _variables;
	}, 
	_object, 
	true, 
	false
] call KH_fnc_execute;

[
	[
		_object,
        _dynamicSimulation,
        _lockedInventory,
        _waterFillPercentage
	], 
	{
		params [
			"_object",
            "_dynamicSimulation",
            "_lockedInventory",
            "_waterFillPercentage"
		];

		_object enableDynamicSimulation _dynamicSimulation;
        _object lockInventory _lockedInventory;
        _object setWaterFillPercentage _waterFillPercentage;
	}, 
	"GLOBAL", 
	true, 
	["JIP", _object, false, ["KH_var_savedUnitAttributes_", [_object, true] call KH_fnc_getEntityVariableName] joinString ""]
] call KH_fnc_execute;

_object;