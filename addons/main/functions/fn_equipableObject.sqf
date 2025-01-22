params ["_unit", "_object", "_bone", "_position", "_rotation", ["_scale", 1], ["_mass", 1], ["_hideInVehicles", true], ["_toggleEquip", true], ["_exclusive", true], ["_event", "KH_eve_objectEquipped"], ["_objectName", "Item"]];

if (isNull _unit) then {
	private _localArguments = [_object, _bone, _position, _rotation, _scale, _mass, _hideInVehicles, _toggleEquip, _exclusive, _event, _objectName];
	
	if _exclusive then {
		_object setVariable ["KH_var_exclusiveObject", true, true];
	};
	
	[
		[
			_object,
			format ["Equip %1", _objectName],
			"\a3\data_f_destroyer\data\UI\IGUI\Cfg\holdactions\holdAction_loadVehicle_ca.paa",
			"\a3\data_f_destroyer\data\UI\IGUI\Cfg\holdactions\holdAction_loadVehicle_ca.paa",
			"(((_this distance _target) < 4) && (isNil {_target getVariable 'KH_var_previouslyEquipped'}))",
			"(((_caller distance _target) < 4) && (!(_caller getVariable ['KH_var_carryingObject', false]) || !(_target getVariable ['KH_var_exclusiveObject', false])))",
			{			
				if ((_caller getVariable ["KH_var_carryingObject", false]) && (_target getVariable ["KH_var_exclusiveObject", false])) then {
					hint "You are already carrying an object that is incompatible with this one.";
				};			
			},
			{},
			{
				private _localArguments = (_this select 3) select 0;
				private _bone = _localArguments select 1;
				private _position = _localArguments select 2;
				private _rotation = _localArguments select 3;
				private _scale = _localArguments select 4;
				private _mass = _localArguments select 5;
				private _hideInVehicles = _localArguments select 6;
				private _toggleEquip = _localArguments select 7;
				private _exclusive = _localArguments select 8;
				private _event = _localArguments select 9;
				private _objectName = _localArguments select 10;
				_target setVariable ["KH_var_previouslyEquipped", true, true];
				[[_caller, _target, _bone, _position, _rotation, _scale, _mass, _hideInVehicles, _toggleEquip, _exclusive, _event, _objectName], "KH_fnc_equipableObject", "SERVER", "THIS_FRAME"] call KH_fnc_execute;

				private _unequipAction = [
					_caller,
					format ["Unequip %1", _objectName],
					"\a3\data_f_destroyer\data\UI\IGUI\Cfg\holdactions\holdAction_unloadVehicle_ca.paa",
					"\a3\data_f_destroyer\data\UI\IGUI\Cfg\holdactions\holdAction_unloadVehicle_ca.paa",
					"true",
					"true",
					{},
					{},
					{
						private _localArguments = (_this select 3) select 0;
						private _object = _localArguments select 0;
						private _event = _localArguments select 9;
						[_event, [_caller, _object, false]] call CBA_fnc_globalEvent;
						detach _object;
					},
					{},
					[_localArguments],
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
						[_localId] call KH_fnc_removeEventHandler;
					}
				] call KH_fnc_addEventHandler;
			},
			{},
			[_localArguments],
			3,
			0,
			false,
			false,
			true
		],
		"BIS_fnc_holdActionAdd", 
		["JIP", "PLAYERS", _object, false, false, ""], 
		"THIS_FRAME"
	] call KH_fnc_execute;
}
else {
	private _localArguments = [_unit, _object, _bone, _position, _rotation, _scale, _mass, _hideInVehicles, _toggleEquip, _exclusive, _event, _objectName];
	[_event, [_unit, _object, true]] call CBA_fnc_globalEvent;

	if (isNil "KH_var_equipableObjectRespawnReset") then {
		KH_var_equipableObjectRespawnReset = true;

		[
			"KH_eve_playerRespawned", 
			{
				params ["_unit"];
				_unit setVariable ["KH_var_carryingObject", false, true];
			}
		] call CBA_fnc_addEventHandler;		
	};
	
	if _exclusive then {
		_unit setVariable ["KH_var_carryingObject", true, true];
	};

	if (_bone != "") then {
		_object attachTo [_unit, _position, _bone, true];
	}
	else {
		_object attachTo [_unit, _position];
	};

	[_object, _rotation, false] call KH_fnc_setRotation;
	
	[
		{
			_args params ["_unit", "_object", "_exclusive"];
			
			if (((attachedTo _object) != _unit) || !(alive _unit)) then {
				["KH_eve_equipableObjectUnequipped", [], _unit] call CBA_fnc_targetEvent;

				if _exclusive then {
					_unit setVariable ["KH_var_carryingObject", false, true];
				};

				[_handle] call CBA_fnc_removePerFrameHandler;
			};
		},
		0.1,
		[_unit, _object, _exclusive]
	] call CBA_fnc_addPerFrameHandler;

	if (_toggleEquip && (isNil {_object getVariable 'KH_var_equipActions'})) then {
		[
			[
				_object,
				format ["Unequip %1 Carried By This Unit", _objectName],
				"\a3\data_f_destroyer\data\UI\IGUI\Cfg\holdactions\holdAction_unloadVehicle_ca.paa",
				"\a3\data_f_destroyer\data\UI\IGUI\Cfg\holdactions\holdAction_unloadVehicle_ca.paa",
				"(((_this distance _target) < 4) && !(isNull (attachedTo _target)))",
				"((_caller distance _target) < 4)",
				{},
				{},
				{
					private _localArguments = (_this select 3) select 0;
					private _unit = _localArguments select 0;
					private _event = _localArguments select 10;
					[_event, [_unit, _target, false]] call CBA_fnc_globalEvent;
					detach _target;
				},
				{},
				[_localArguments],
				3,
				0,
				false,
				false,
				false
			],
			"BIS_fnc_holdActionAdd", 
			["JIP", "PLAYERS", _object, false, false, ""], 
			"THIS_FRAME"
		] call KH_fnc_execute;
		
		[
			[
				_object,
				format ["Equip %1", _objectName],
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
					private _localArguments = (_this select 3) select 0;
					private _bone = _localArguments select 2;
					private _position = _localArguments select 3;
					private _rotation = _localArguments select 4;
					private _scale = _localArguments select 5;
					private _mass = _localArguments select 6;
					private _hideInVehicles = _localArguments select 7;
					private _toggleEquip = _localArguments select 8;
					private _exclusive = _localArguments select 9;
					private _event = _localArguments select 10;
					private _objectName = _localArguments select 11;
					[[_caller, _target, _bone, _position, _rotation, _scale, _mass, _hideInVehicles, _toggleEquip, _exclusive, _event, _objectName], "KH_fnc_equipableObject", "SERVER", "THIS_FRAME"] call KH_fnc_execute;

					private _unequipAction = [
						_caller,
						format ["Unequip %1", ((_this select 3) select 0) select 11],
						"\a3\data_f_destroyer\data\UI\IGUI\Cfg\holdactions\holdAction_unloadVehicle_ca.paa",
						"\a3\data_f_destroyer\data\UI\IGUI\Cfg\holdactions\holdAction_unloadVehicle_ca.paa",
						"true",
						"true",
						{},
						{},
						{
							private _localArguments = (_this select 3) select 0;
							private _object = _localArguments select 1;
							private _event = _localArguments select 10;
							[_event, [_caller, _object, false]] call CBA_fnc_globalEvent;
							detach _object;
						},
						{},
						[_localArguments],
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
							[_localId] call KH_fnc_removeEventHandler;
						}
					] call KH_fnc_addEventHandler;
				},
				{},
				[_localArguments],
				3,
				0,
				false,
				false,
				false
			],
			"BIS_fnc_holdActionAdd", 
			["JIP", "PLAYERS", _object, false, false, ""], 
			"THIS_FRAME"
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
			["JIP", "GLOBAL", _object, false, false, ""], 
			"THIS_FRAME"
		] call KH_fnc_execute;
		
		if _hideInVehicles then {			
			[
				{
					_args params ["_object"];
					
					if !(isNull (objectParent (attachedTo _object))) then {
						_object hideObjectGlobal true;
					}
					else {
						_object hideObjectGlobal false;
					};
				},
				0.1,
				[_object]
			] call CBA_fnc_addPerFrameHandler;
		};
		
		_object setVariable ["KH_var_equipActions", true];
	};
};

true;