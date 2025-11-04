params [
	["_unit", objNull, [objNull]], 
	["_object", objNull, [objNull]], 
	["_bone", "", [""]], 
	["_position", [], [[]]], 
	["_rotation", [], [[], objNull]], 
	["_scale", 1, [0]], 
	["_disableCollision", true, [true]], 
	["_hideInVehicles", true, [true]], 
	["_exclusive", true, [true]], 
	["_name", "", [""]]
];

if (isNil {_object getVariable "KH_var_equipableObjectSet";}) then {
	if (_name isEqualTo "") then {
		_name = getText (configFile >> "CfgVehicles" >> (typeOf _object) >> "displayName");
	};

	_object setVariable ["KH_var_equipableObjectSet", true, true];

	[
		[_object, false],
		["<img image='\A3\ui_f\data\igui\cfg\actions\take_ca.paa' size='1.8'/><br/>EQUIP ", _name] joinString "",
		[objNull, _object, _bone, _position, _rotation, _scale, _disableCollision, _hideInVehicles, _exclusive, _name],
		{
			_this set [0, _caller];
			call KH_fnc_equipableObject;
		},
		{
			(isNull (attachedTo _target));
		},
		true,
		true,
		"PLAYERS",
		false,
		2,
		true,
		false,
		false,
		true,
		true,
		"",
		"",
		false
	] call KH_fnc_addAction;

	[
		[_object, false],
		["<img image='\A3\ui_f\data\igui\cfg\actions\take_ca.paa' size='1.8'/><br/>UNEQUIP ", _name, " FROM ", name (attachedTo _object)] joinString "",
		[_exclusive],
		{
			params ["_exclusive"];
			private _carrier = attachedTo _target;
			["KH_eve_equipableObjectExchanged", [_carrier, _target, false]] call CBA_fnc_globalEvent;

			if _exclusive then {
				if (_carrier getVariable ["KH_var_equipableObjectExclusive", false]) then {
					_carrier setVariable ["KH_var_equipableObjectExclusive", false, true];
				};
			};

			detach _target;
		},
		{
			!(isNull (attachedTo _target));
		},
		true,
		true,
		"PLAYERS",
		false,
		2,
		true,
		false,
		false,
		true,
		true,
		"",
		"",
		false
	] call KH_fnc_addAction;

	[
		false,
		["<img image='\A3\ui_f\data\igui\cfg\actions\take_ca.paa' size='1.8'/><br/>UNEQUIP ", _name] joinString "",
		[_object, _exclusive],
		{
			params ["_object", "_exclusive"];
			["KH_eve_equipableObjectExchanged", [_caller, _object, false]] call CBA_fnc_globalEvent;

			if _exclusive then {
				if (_caller getVariable ["KH_var_equipableObjectExclusive", false]) then {
					_caller setVariable ["KH_var_equipableObjectExclusive", false, true];
				};
			};

			detach _object;
		},
		{
			params ["_object"];
			((attachedTo _object) isEqualTo _caller);
		},
		true,
		true,
		"PLAYERS",
		false,
		2,
		true,
		false,
		false,
		true,
		true,
		"",
		"",
		false
	] call KH_fnc_addAction;
};

if !(isNull _unit) then {
	if (_unit getVariable ["KH_var_equipableObjectExclusive", false]) exitWith {};
	["KH_eve_equipableObjectExchanged", [_unit, _object, true]] call CBA_fnc_globalEvent;

	if _exclusive then {
		_unit setVariable ["KH_var_equipableObjectExclusive", true, true];
	};

	if (_bone isNotEqualTo "") then {
		_object attachTo [_unit, _position, _bone, true];
	}
	else {
		_object attachTo [_unit, _position];
	};

	_object setObjectScale _scale;

	if (_rotation isEqualType objNull) then {
		_object setVectorDirAndUp [vectorDir _rotation, vectorUp _rotation];
	}
	else {
		if (_rotation isEqualTypeAll []) then {
			_object setVectorDirAndUp _rotation;
		}
		else {
			_object setRotationEuler _rotation;
		};
	};

	[
		[_object, _rotation],
		{
			params ["_object", "_rotation"];

			if (_rotation isEqualType objNull) then {
				_object setVectorDirAndUp [vectorDir _rotation, vectorUp _rotation];
			}
			else {
				if (_rotation isEqualTypeAll []) then {
					_object setVectorDirAndUp _rotation;
				}
				else {
					_object setRotationEuler _rotation;
				};
			};
		},
		_object,
		true,
		false
	] call KH_fnc_execute;

	[
		[_unit, _object, _exclusive],
		{
			params ["_unit", "_object", "_exclusive"];
			
			if (((attachedTo _object) isNotEqualTo _unit) || !(alive _unit)) then {
				if _exclusive then {
					_unit setVariable ["KH_var_carryingObject", false, true];
				};

				[_handlerId] call KH_fnc_removeHandler;
			};
		},
		true,
		0,
		false
	] call KH_fnc_execute;

	if (_hideInVehicles || _disableCollision) then {	
		[
			[_object, _hideInVehicles, _disableCollision],
			{
				params ["_object", "_hideInVehicles", "_disableCollision"];
				
				if (isNil {_object getVariable "KH_var_equipableObjectHandler";}) then { 
					private _handler = [
						[_object, _hideInVehicles, _disableCollision],
						{
							params ["_object", "_hideInVehicles", "_disableCollision"];

							if _disableCollision then {
								if (getPhysicsCollisionFlag _object) then {
									[
										[_object],
										{
											params ["_object"];
											_object setPhysicsCollisionFlag false;
										},
										"GLOBAL",
										true,
										["JIP", _object, false, ["KH_var_equipableObjectCollision", _object] joinString "_"]
									] call KH_fnc_execute;
								}
								else {
									[
										[_object],
										{
											params ["_object"];
											_object setPhysicsCollisionFlag true;
										},
										"GLOBAL",
										true,
										["JIP", _object, false, ["KH_var_equipableObjectCollision", _object] joinString "_"]
									] call KH_fnc_execute;
								};
							};

							if _hideInVehicles then {
								if !(isNull (objectParent (attachedTo _object))) then {
									_object hideObjectGlobal true;
								}
								else {
									_object hideObjectGlobal false;
								};
							};
						},
						true,
						0,
						false
					] call KH_fnc_execute;

					_object setVariable ["KH_var_equipableObjectHandler", _handler];
				};
			},
			"SERVER",
			true,
			false
		] call KH_fnc_execute;
	};
};

nil;