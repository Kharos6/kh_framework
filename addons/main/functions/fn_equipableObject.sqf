params [
	["_unit", objNull, [objNull]], 
	["_object", objNull, [objNull]], 
	["_bone", "", [""]], 
	["_position", [], [[]]], 
	["_rotation", [], [[], objNull]], 
	["_scale", 1, [0]], 
	["_mass", 1, [0]], 
	["_hideInVehicles", true, [true]], 
	["_toggleEquip", true, [true]], 
	["_exclusive", true, [true]], 
	["_objectName", "Item", [""]]
];

if (isNull _unit) then {
	private _localArguments = _this select [1];
	
	if _exclusive then {
		_object setVariable ["KH_var_exclusiveObject", true, true];
	};
	
	[
		[
			_object,
			["Equip ", _objectName] joinString "",
			"\a3\data_f_destroyer\data\UI\IGUI\Cfg\holdactions\holdAction_loadVehicle_ca.paa",
			"\a3\data_f_destroyer\data\UI\IGUI\Cfg\holdactions\holdAction_loadVehicle_ca.paa",
			"(((_this distance _target) < 4) && (isNil {_target getVariable 'KH_var_previouslyEquipped';}))",
			"(((_caller distance _target) < 4) && (!(_caller getVariable ['KH_var_carryingObject', false]) || !(_target getVariable ['KH_var_exclusiveObject', false])))",
			{			
				if ((_caller getVariable ["KH_var_carryingObject", false]) && (_target getVariable ["KH_var_exclusiveObject", false])) then {
					hint "You are already carrying an object that is incompatible with this one.";
				};			
			},
			{},
			{
				private _localArguments = param [3];
				(_localArguments select [1]) params ["_bone", "_position", "_rotation", "_scale", "_mass", "_hideInVehicles", "_toggleEquip", "_exclusive", "_objectName"];
				_target setVariable ["KH_var_previouslyEquipped", true, true];

				[
					[_caller, _target, _bone, _position, _rotation, _scale, _mass, _hideInVehicles, _toggleEquip, _exclusive, _objectName], 
					"KH_fnc_equipableObject", 
					"SERVER", 
					true,
					false
				] call KH_fnc_execute;

				private _unequipAction = [
					_caller,
					["Unequip ", _objectName] joinString "",
					"\a3\data_f_destroyer\data\UI\IGUI\Cfg\holdactions\holdAction_unloadVehicle_ca.paa",
					"\a3\data_f_destroyer\data\UI\IGUI\Cfg\holdactions\holdAction_unloadVehicle_ca.paa",
					"true",
					"true",
					{},
					{},
					{
						private _localArguments = param [3];
						private _object = _localArguments param [0];
						["KH_eve_equipableObjectExchanged", [_caller, _object, false]] call CBA_fnc_globalEvent;
						detach _object;
					},
					{},
					_localArguments,
					3,
					0,
					false,
					false,
					false
				] call BIS_fnc_holdActionAdd;
				
				[
					"CBA",
					"KH_eve_equipableObjectUnequipped",
					[_unequipAction],
					{
						_args params ["_unequipAction"];
						[player, _unequipAction] call BIS_fnc_holdActionRemove;
						[_handlerId] call KH_fnc_removeHandler;
					}
				] call KH_fnc_addEventHandler;
			},
			{},
			_localArguments,
			3,
			0,
			false,
			false,
			true
		],
		"BIS_fnc_holdActionAdd",
		"PLAYERS",
		true,
		["JIP", "PLAYERS", _object, false, false, ""]
	] call KH_fnc_execute;
}
else {
	private _localArguments = _this;
	["KH_eve_equipableObjectExchanged", [_unit, _object, true]] call CBA_fnc_globalEvent;

	if (isNil "KH_var_equipableObjectRespawnReset") then {
		KH_var_equipableObjectRespawnReset = true;

		[
			"KH_eve_playerRespawned", 
			{
				private _unit = param [3];
				_unit setVariable ["KH_var_carryingObject", false, true];
			}
		] call CBA_fnc_addEventHandler;		
	};
	
	if _exclusive then {
		_unit setVariable ["KH_var_carryingObject", true, true];
	};

	if (_bone isNotEqualTo "") then {
		_object attachTo [_unit, _position, _bone, true];
	}
	else {
		_object attachTo [_unit, _position];
	};

	if (_rotation isEqualType objNull) then {
		_object setVectorDirAndUp [_rotation select 0, _rotation select 1];
	}
	else {
		_object setVectorDirAndUp _rotation;
	};
	
	[
		[_unit, _object, _exclusive],
		{
			params ["_unit", "_object", "_exclusive"];
			
			if (((attachedTo _object) isNotEqualTo _unit) || !(alive _unit)) then {
				["KH_eve_equipableObjectUnequipped", [], _unit] call CBA_fnc_targetEvent;

				if _exclusive then {
					_unit setVariable ["KH_var_carryingObject", false, true];
				};

				[_handlerId] call KH_fnc_removeHandler;
			};
		},
		true,
		0.1,
		false
	] call KH_fnc_execute;

	if (_toggleEquip && (isNil {_object getVariable "KH_var_equipActions";})) then {
		[
			[
				_object,
				["Unequip ", _objectName, " carried by this unit."] joinString "",
				"\a3\data_f_destroyer\data\UI\IGUI\Cfg\holdactions\holdAction_unloadVehicle_ca.paa",
				"\a3\data_f_destroyer\data\UI\IGUI\Cfg\holdactions\holdAction_unloadVehicle_ca.paa",
				"(((_this distance _target) < 4) && !(isNull (attachedTo _target)))",
				"((_caller distance _target) < 4)",
				{},
				{},
				{
					private _localArguments = param [3];
					private _unit = _localArguments param [0];
					["KH_eve_equipableObjectExchanged", [_unit, _target, false]] call CBA_fnc_globalEvent;
					detach _target;
				},
				{},
				_localArguments,
				3,
				0,
				false,
				false,
				false
			],
			"BIS_fnc_holdActionAdd",
			"PLAYERS",
			true,
			["JIP", "PLAYERS", _object, false, false, ""]
		] call KH_fnc_execute;
		
		[
			[
				_object,
				["Equip ", _objectName] joinString "",
				"\a3\data_f_destroyer\data\UI\IGUI\Cfg\holdactions\holdAction_loadVehicle_ca.paa",
				"\a3\data_f_destroyer\data\UI\IGUI\Cfg\holdactions\holdAction_loadVehicle_ca.paa",
				"(((_this distance _target) < 4) && (isNull (attachedTo _target)))",
				"(((_caller distance _target) < 4) && (!(_caller getVariable ['KH_var_carryingObject', false]) || !(_target getVariable ['KH_var_exclusiveObject', false])))",
				{							
					if ((_caller getVariable ["KH_var_carryingObject", false]) && (_target getVariable ["KH_var_exclusiveObject", false])) then {
						hint "You are already carrying an object that is incompatible with this one.";
					};			
				},
				{},
				{
					private _localArguments = param [3];
					(_localArguments select [2]) params ["_bone", "_position", "_rotation", "_scale", "_mass", "_hideInVehicles", "_toggleEquip", "_exclusive", "_objectName"];

					[
						[_caller, _target, _bone, _position, _rotation, _scale, _mass, _hideInVehicles, _toggleEquip, _exclusive, _objectName], 
						"KH_fnc_equipableObject", 
						"SERVER", 
						true,
						false
					] call KH_fnc_execute;

					private _unequipAction = [
						_caller,
						["Unequip ", _objectName] joinString "",
						"\a3\data_f_destroyer\data\UI\IGUI\Cfg\holdactions\holdAction_unloadVehicle_ca.paa",
						"\a3\data_f_destroyer\data\UI\IGUI\Cfg\holdactions\holdAction_unloadVehicle_ca.paa",
						"true",
						"true",
						{},
						{},
						{
							private _localArguments = param [3];
							private _object = _localArguments param [1];
							["KH_eve_equipableObjectExchanged", [_caller, _object, false]] call CBA_fnc_globalEvent;
							detach _object;
						},
						{},
						_localArguments,
						3,
						0,
						false,
						false,
						false
					] call BIS_fnc_holdActionAdd;
					
					[
						"CBA",
						"KH_eve_equipableObjectUnequipped",
						[_unequipAction],
						{
							_args params ["_unequipAction"];
							[player, _unequipAction] call BIS_fnc_holdActionRemove;
							[_handlerId] call KH_fnc_removeHandler;
						}
					] call KH_fnc_addEventHandler;
				},
				{},
				_localArguments,
				3,
				0,
				false,
				false,
				false
			],
			"BIS_fnc_holdActionAdd",
			"PLAYERS",
			true,
			["JIP", _object, false, ""]
		] call KH_fnc_execute;
		
		if _exclusive then {
			_object setVariable ["KH_var_exclusiveObject", true, true];
		};
		
		[
			[_object, _scale, _mass],
			{
				params ["_object", "_scale", "_mass"];
				_object setObjectScale _scale;
				_object setMass _mass;
			},
			"GLOBAL",
			true,
			["JIP", _object, false, ""]
		] call KH_fnc_execute;
		
		if _hideInVehicles then {	
			[
				[_object],
				{
					_args params ["_object"];
					
					if !(isNull (objectParent (attachedTo _object))) then {
						_object hideObjectGlobal true;
					}
					else {
						_object hideObjectGlobal false;
					};
				},
				true,
				0.1,
				false
			] call KH_fnc_execute;
		};
		
		_object setVariable ["KH_var_equipActions", true];
	};
};

nil;