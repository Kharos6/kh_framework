params ["_object", "_vehicle", "_height", "_distance", "_maximumParticipants", "_duration", ["_objectName", ""]];
_height = _height min 100;
private _fultonEvent = format ["KH_eve_%1", [0, 36, "ALPHANUMERIC"] call KH_fnc_generateSymbols];
private _fultonId = [missionNamespace, "KH_var_fultonId", false, true] call KH_fnc_atomicVariable;

[
	[
		_object,
		format ["Prime Fulton %1", _objectName],
		"\a3\data_f_destroyer\data\UI\IGUI\Cfg\holdactions\holdAction_loadVehicle_ca.paa",
		"\a3\data_f_destroyer\data\UI\IGUI\Cfg\holdactions\holdAction_loadVehicle_ca.paa",
		["(!(missionNamespace getVariable ['", _fultonId, "', false]) && ((_this distance _target) < 4))"] joinString "",
		["(!(missionNamespace getVariable ['", _fultonId, "', false]) && ((_caller distance _target) < 4))"] joinString "",
		{},
		{},
		{
			(_this select 3) params ["_fultonEvent"];
			[_fultonEvent, []] call CBA_fnc_serverEvent;
		},
		{},
		[_fultonEvent],
		6,
		0,
		false,
		false,
		true
	],
	"BIS_fnc_holdActionAdd", 
	["JIP", "PLAYERS", _object, false, false], 
	"THIS_FRAME"
] call KH_fnc_execute;

[
	["CBA"],
	_fultonEvent,
	[_object, _vehicle, _height, _distance, _maximumParticipants, _duration, _objectName, _fultonId],
	{
		_args params ["_object", "_vehicle", "_height", "_distance", "_maximumParticipants", "_duration", "_objectName", "_fultonId"];
		missionNamespace setVariable [_fultonId, true, true];
		private _mainFulton = createVehicle ["B_Parachute_02_F", _object, [], 0, "CAN_COLLIDE"];
		_mainFulton allowDamage false;
		private _fultonActive = [missionNamespace, "KH_var_fultonActive", false, true] call KH_fnc_atomicVariable;
		private _currentFultonParticipants = [missionNamespace, "KH_var_currentFultonParticipants", [], true] call KH_fnc_atomicVariable;
		private _mainFultonVelocity = [missionNamespace, "KH_var_fultonExtractVelocity", 0.1, false] call KH_fnc_atomicVariable;
		private _fultonRemoteActions = [missionNamespace, "KH_var_fultonRemoteActions", false, false] call KH_fnc_atomicVariable;

		private _fultonFunction = {
			params ["_object", "_objectName", "_mainFulton", "_currentFultonParticipants", "_participant"];
			_participant setVariable ["KH_var_fultonAttached", true, true];

			[
				[_object, _mainFulton, _currentFultonParticipants, _participant],
				{
					params ["_object", "_mainFulton", "_currentFultonParticipants", "_participant"];

					if (isNil "KH_var_fultonRespawnReset") then {
						KH_var_fultonRespawnReset = true;

						[
							"KH_eve_playerRespawned", 
							{
								params ["_unit"];
								_unit setVariable ["KH_var_fultonAttached", false, true];
								["KH_eve_fultonDetached", [], _unit] call CBA_fnc_targetEvent;
							}
						] call CBA_fnc_addEventHandler;		
					};

					private _currentParticipants = missionNamespace getVariable [_currentFultonParticipants, []];
					_currentParticipants pushBackUnique _participant;
					missionNamespace setVariable [_currentFultonParticipants, _currentParticipants, true];
					_anchorEnd = createVehicle ["Land_Can_V2_F", _participant, [], 0, "CAN_COLLIDE"];
					_anchorEnd allowDamage false;
					_anchorEnd attachTo [_participant, [0, 0, 0], "Spine3"];
					private _rope = ropeCreate [_mainFulton, [0, 0, 0], _anchorEnd, [0, 0, 0], 100];
					_rope allowDamage false;

					[
						{
							private _object = _this select 0;
							private _participant = _this select 1;
							private _mainFulton = _this select 2;
							private _rope = _this select 4;

							if (isNil "_rope") then {
								true;
							}
							else {
								if (!(isNull (objectParent _participant)) || !(_participant getVariable ["KH_var_fultonAttached", false]) || (isNull _rope) || !(alive _participant) || ((ropeAttachedObjects ropesOwner) isEqualTo [])) then {
									true;
								}
								else {
									false;
								};
							};
						}, 
						{
							private _participant = _this select 1;
							private _currentFultonParticipants = _this select 3;
							private _anchorEnd = _this select 4;
							private _rope = _this select 5;
							private _currentParticipants = missionNamespace getVariable [_currentFultonParticipants, []];

							if (_participant in _currentParticipants) then {
								_currentParticipants deleteAt (_currentParticipants find _participant);
								missionNamespace setVariable [_currentFultonParticipants, _currentParticipants, true];
							};
							
							_participant setVariable ["KH_var_fultonAttached", false, true];
							ropeDestroy _rope;
							deleteVehicle _anchorEnd;

							if (isPlayer _participant) then {
								["KH_eve_fultonDetached", [], _participant] call CBA_fnc_targetEvent;
							};
						}, 
						[_object, _participant, _mainFulton, _currentFultonParticipants, _anchorEnd, _rope]
					] call CBA_fnc_waitUntilAndExecute;
				},
				"SERVER",
				"THIS_FRAME"
			] call KH_fnc_execute;

			if (isPlayer _participant) then {
				private _detachAction = [
					_participant,
					format ["Detach Self From Fulton %1", _objectName],
					"\a3\data_f_destroyer\data\UI\IGUI\Cfg\holdactions\holdAction_unloadVehicle_ca.paa",
					"\a3\data_f_destroyer\data\UI\IGUI\Cfg\holdactions\holdAction_unloadVehicle_ca.paa",
					"(_this getVariable ['KH_var_fultonAttached', false])",
					"(_caller getVariable ['KH_var_fultonAttached', false])",
					{},
					{},
					{
						_caller setVariable ["KH_var_fultonAttached", false, true];
						["KH_eve_fultonDetached", []] call CBA_fnc_localEvent;
					},
					{},
					[],
					3,
					0,
					false,
					false,
					false
				] call BIS_fnc_holdActionAdd; 

				[
					["CBA"],
					"KH_eve_fultonDetached",
					[_detachAction],
					{
						_args params ["_detachAction"];
						[player, _detachAction] call BIS_fnc_holdActionRemove;
						[_thisType, _thisId] call CBA_fnc_removeEventHandler;
					}
				] call KH_fnc_addEventHandler;
			};
		};

		[
			[
				_object,
				format ["Attach Self To Fulton %1", _objectName],
				"\a3\data_f_destroyer\data\UI\IGUI\Cfg\holdactions\holdAction_loadVehicle_ca.paa",
				"\a3\data_f_destroyer\data\UI\IGUI\Cfg\holdactions\holdAction_loadVehicle_ca.paa",
				["(((count (missionNamespace getVariable ['", _currentFultonParticipants, "', []])) < ", _maximumParticipants, ") && !(_this getVariable ['KH_var_fultonAttached', false]) && ((_this distance _target) < 6))"] joinString "",
				["(((count (missionNamespace getVariable ['", _currentFultonParticipants, "', []])) < ", _maximumParticipants, ") && !(_caller getVariable ['KH_var_fultonAttached', false]) && ((_caller distance _target) < 6))"] joinString "",
				{},
				{},
				{
					(_this select 3) params ["_objectName", "_mainFulton", "_currentFultonParticipants", "_fultonFunction"];
					[_target, _objectName, _mainFulton, _currentFultonParticipants, _caller] call _fultonFunction;
				},
				{},
				[_objectName, _mainFulton, _currentFultonParticipants, _fultonFunction],
				3,
				0,
				false,
				false,
				true
			],
			"BIS_fnc_holdActionAdd", 
			["JIP", "PLAYERS", _object, false, false], 
			"THIS_FRAME"
		] call KH_fnc_execute;

		[
			{
				_args params ["_object", "_maximumParticipants", "_objectName", "_mainFulton", "_fultonActive", "_currentFultonParticipants", "_fultonFunction", "_fultonRemoteActions"];
				
				if !(missionNamespace getVariable [_fultonActive, false]) then {
					{
						if !(_x getVariable [_fultonRemoteActions, false]) then {
							_x setVariable [_fultonRemoteActions, true];

							[
								[
									_x,
									format ["Attach %1 To Fulton %2", name _x, _objectName],
									"\a3\data_f_destroyer\data\UI\IGUI\Cfg\holdactions\holdAction_loadVehicle_ca.paa",
									"\a3\data_f_destroyer\data\UI\IGUI\Cfg\holdactions\holdAction_loadVehicle_ca.paa",
									["(((count (missionNamespace getVariable ['", _currentFultonParticipants, "', []])) < ", _maximumParticipants, ") && !(_target getVariable ['KH_var_fultonAttached', false]) && ((_target distance ", _object, ") < 6) && (_target isNotEqualTo _this) && ((_this distance _target) < 4) && (alive _target))"] joinString "",
									["(((count (missionNamespace getVariable ['", _currentFultonParticipants, "', []])) < ", _maximumParticipants, ") && !(_target getVariable ['KH_var_fultonAttached', false]) && ((_target distance ", _object, ") < 6) && (_target isNotEqualTo _caller) && ((_caller distance _target) < 4) && (alive _target))"] joinString "",
									{},
									{},
									{
										(_this select 3) params ["_object", "_objectName", "_mainFulton", "_currentFultonParticipants", "_fultonFunction"];
										[_object, _objectName, _mainFulton, _currentFultonParticipants, _target] call _fultonFunction;
									},
									{},
									[_object, _objectName, _mainFulton, _currentFultonParticipants, _fultonFunction],
									3,
									0,
									false,
									false,
									true
								],
								"BIS_fnc_holdActionAdd", 
								["JIP", "PLAYERS", _x, false, false], 
								"THIS_FRAME"
							] call KH_fnc_execute;

							[
								[
									_x,
									format ["Detach %1 From Fulton %2", name _x, _objectName],
									"\a3\data_f_destroyer\data\UI\IGUI\Cfg\holdactions\holdAction_unloadVehicle_ca.paa",
									"\a3\data_f_destroyer\data\UI\IGUI\Cfg\holdactions\holdAction_unloadVehicle_ca.paa",
									"((_target getVariable ['KH_var_fultonAttached', false]) && (_target isNotEqualTo _this) && ((_this distance _target) < 4) && (alive _target))",
									"((_target getVariable ['KH_var_fultonAttached', false]) && (_target isNotEqualTo _caller) && ((_caller distance _target) < 4) && (alive _target))",
									{},
									{},
									{
										_target setVariable ["KH_var_fultonAttached", false, true];

										if (isPlayer _target) then {
											["KH_eve_fultonDetached", [], _target] call CBA_fnc_targetEvent;
										};
									},
									{},
									[],
									3,
									0,
									false,
									false,
									true
								],
								"BIS_fnc_holdActionAdd", 
								["JIP", "PLAYERS", _x, false, false], 
								"THIS_FRAME"
							] call KH_fnc_execute;
						};
					} forEach (_object nearEntities ["Man", 15]);
				}
				else {
					[_handle] call CBA_fnc_removePerFrameHandler;
				};
			},
			1,
			[_object, _maximumParticipants, _objectName, _mainFulton, _fultonActive, _currentFultonParticipants, _fultonFunction, _fultonRemoteActions]
		] call CBA_fnc_addPerFrameHandler;

		[
			{
				params ["_object", "_vehicle", "_height", "_distance", "_duration", "_mainFulton", "_fultonActive", "_currentFultonParticipants", "_mainFultonVelocity"];

				[
					{
						_args params ["_object", "_vehicle", "_height", "_distance", "_duration", "_mainFulton", "_fultonActive", "_currentFultonParticipants", "_mainFultonVelocity"];
						
						if ((_vehicle distance _mainFulton) < _distance) then {
							{
								[
									[_x, _vehicle, _duration],
									{
										params ["_unit", "_vehicle", "_duration"];
										_unit setVariable ["KH_var_fultonAttached", false, true];
										private _fulton = createVehicle ["NonSteerable_Parachute_F", _unit, [], 0, "CAN_COLLIDE"];
										_fulton allowDamage false;
										_unit allowDamage false;
										_unit moveInAny _fulton;

										if (isPlayer _unit) then {
											addCamShake [2, _duration, 18];
										};

										[
											{
												_args params ["_unit", "_vehicle", "_duration", "_fulton"];
												private _remainingTime = _fulton getVariable ["KH_var_fultonRemainingTime", _duration];
												private _timeStep = diag_deltaTime;
												_remainingTime = _remainingTime - _timeStep;
												_fulton setVariable ["KH_var_fultonRemainingTime", _remainingTime];

												if ((_remainingTime <= 0) || ((_unit distance _vehicle) < 30)) then {
													moveOut _unit;
													deleteVehicle _fulton;

													[
														{
															params ["_unit"];
															(isNull (objectParent _unit));
														}, 
														{
															params ["_unit", "_vehicle"];

															[
																{
																	params ["_unit", "_vehicle"];
																	_unit moveInAny _vehicle;
																	_unit allowDamage true;
																}, 
																[_unit, _vehicle]
															] call CBA_fnc_execNextFrame;
														}, 
														[_unit, _vehicle],
														15
													] call CBA_fnc_waitUntilAndExecute;

													[_handle] call CBA_fnc_removePerFrameHandler;
												} 
												else {
													private _fultonPosition = getPosATL _fulton;
													private _vehiclePosition = getPosATL _vehicle;
													private _vehicleRotation = [_vehicle, objNull] call KH_fnc_getRotation;
													private _relativeRotation = [_fulton, _vehicle] call KH_fnc_getRotation;
													private _velocity = (_vehiclePosition vectorDiff _fultonPosition) vectorMultiply (3 / _remainingTime);
													[_fulton, [-30, 0, (_relativeRotation select 2) + 180], false] call KH_fnc_setRotation;
													_fulton setVelocity _velocity;
													_fulton setAngularVelocity [0, 0, 0];
												};
											},
											0,
											[_unit, _vehicle, _duration, _fulton]
										] call CBA_fnc_addPerFrameHandler;
									},
									_x,
									"THIS_FRAME"
								] call KH_fnc_execute;
							} forEach (missionNamespace getVariable [_currentFultonParticipants, []]);

							missionNamespace setVariable [_fultonActive, true, true];
							deleteVehicle _mainFulton;
							deleteVehicle _object;
							[_handle] call CBA_fnc_removePerFrameHandler;
						};

						if !(missionNamespace getVariable [_fultonActive, false]) then {
							if (((getPosATL _mainFulton) select 2) < _height) then {
								private _velocity = [missionNamespace, _mainFultonVelocity, 0.02, false] call KH_fnc_atomicValue;
								_mainFulton setVelocity [0, 0, _velocity];
							}
							else {
								_mainFulton setVelocity [0, 0, 0];
							};
						};
					},
					0,
					[_object, _vehicle, _height, _distance, _duration, _mainFulton, _fultonActive, _currentFultonParticipants, _mainFultonVelocity]
				] call CBA_fnc_addPerFrameHandler;
			}, 
			[_object, _vehicle, _height, _distance, _duration, _mainFulton, _fultonActive, _currentFultonParticipants, _mainFultonVelocity], 
			3
		] call CBA_fnc_waitAndExecute;
	}
] call KH_fnc_addEventHandler;

true;